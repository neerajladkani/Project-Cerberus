// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT license.

#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include "testing.h"
#include "flash/flash_common.h"


static const char *SUITE = "flash_common";


/*******************
 * Test cases
 *******************/

static void flash_common_test_address_to_int (CuTest *test)
{
	uint8_t bytes[4] = {0};
	uint32_t address;

	TEST_START;

	address = flash_address_to_int (bytes, 3);
	CuAssertIntEquals (test, 0, address);
	CuAssertTrue (test, !ROT_IS_ERROR (address));

	bytes[0] = 0x20;
	address = flash_address_to_int (bytes, 3);
	CuAssertIntEquals (test, 0x200000, address);
	CuAssertTrue (test, !ROT_IS_ERROR (address));

	bytes[1] = 0x10;
	address = flash_address_to_int (bytes, 3);
	CuAssertIntEquals (test, 0x201000, address);
	CuAssertTrue (test, !ROT_IS_ERROR (address));

	bytes[2] = 0x30;
	address = flash_address_to_int (bytes, 3);
	CuAssertIntEquals (test, 0x201030, address);
	CuAssertTrue (test, !ROT_IS_ERROR (address));

	address = 0;
	bytes[3] = 0x40;
	address = flash_address_to_int (bytes, 3);
	CuAssertIntEquals (test, 0x201030, address);
	CuAssertTrue (test, !ROT_IS_ERROR (address));
}

static void flash_common_test_address_to_int_4bytes (CuTest *test)
{
	uint8_t bytes[4] = {0};
	uint32_t address;

	TEST_START;

	address = flash_address_to_int (bytes, 4);
	CuAssertIntEquals (test, 0, address);
	CuAssertTrue (test, !ROT_IS_ERROR (address));

	bytes[0] = 0x20;
	address = flash_address_to_int (bytes, 4);
	CuAssertIntEquals (test, 0x20000000, address);
	CuAssertTrue (test, !ROT_IS_ERROR (address));

	bytes[1] = 0x10;
	address = flash_address_to_int (bytes, 4);
	CuAssertIntEquals (test, 0x20100000, address);
	CuAssertTrue (test, !ROT_IS_ERROR (address));

	bytes[2] = 0x30;
	address = flash_address_to_int (bytes, 4);
	CuAssertIntEquals (test, 0x20103000, address);
	CuAssertTrue (test, !ROT_IS_ERROR (address));

	address = 0;
	bytes[3] = 0x40;
	address = flash_address_to_int (bytes, 4);
	CuAssertIntEquals (test, 0x20103040, address);
	CuAssertTrue (test, !ROT_IS_ERROR (address));
}

static void flash_common_test_address_to_int_null (CuTest *test)
{
	uint32_t address;

	TEST_START;

	address = flash_address_to_int (NULL, 3);
	CuAssertIntEquals (test, FLASH_COMMON_INVALID_ARGUMENT, address);
	CuAssertTrue (test, ROT_IS_ERROR (address));
}

static void flash_common_test_address_to_int_bad_length (CuTest *test)
{
	uint8_t bytes[4] = {0x11, 0x22, 0x33, 0x44};
	uint32_t address;

	TEST_START;

	address = flash_address_to_int (bytes, 2);
	CuAssertIntEquals (test, FLASH_COMMON_INVALID_ARGUMENT, address);
	CuAssertTrue (test, ROT_IS_ERROR (address));

	address = flash_address_to_int (bytes, 5);
	CuAssertIntEquals (test, FLASH_COMMON_INVALID_ARGUMENT, address);
	CuAssertTrue (test, ROT_IS_ERROR (address));
}

static void flash_common_test_int_to_address (CuTest *test)
{
	uint32_t address;
	uint8_t bytes[4] = {0};
	int status;

	TEST_START;

	address = 0x1;
	status = flash_int_to_address (address, 3, bytes);
	CuAssertIntEquals (test, 0, status);
	CuAssertIntEquals (test, 0x00, bytes[0]);
	CuAssertIntEquals (test, 0x00, bytes[1]);
	CuAssertIntEquals (test, 0x01, bytes[2]);

	address = 0x1002;
	status = flash_int_to_address (address, 3, bytes);
	CuAssertIntEquals (test, 0, status);
	CuAssertIntEquals (test, 0x00, bytes[0]);
	CuAssertIntEquals (test, 0x10, bytes[1]);
	CuAssertIntEquals (test, 0x02, bytes[2]);

	address = 0x341200;
	status = flash_int_to_address (address, 3, bytes);
	CuAssertIntEquals (test, 0, status);
	CuAssertIntEquals (test, 0x34, bytes[0]);
	CuAssertIntEquals (test, 0x12, bytes[1]);
	CuAssertIntEquals (test, 0x00, bytes[2]);

	address = 0x11223344;
	status = flash_int_to_address (address, 3, bytes);
	CuAssertIntEquals (test, 0, status);
	CuAssertIntEquals (test, 0x22, bytes[0]);
	CuAssertIntEquals (test, 0x33, bytes[1]);
	CuAssertIntEquals (test, 0x44, bytes[2]);
}

static void flash_common_test_int_to_address_4byte (CuTest *test)
{
	uint32_t address;
	uint8_t bytes[4] = {0};
	int status;

	TEST_START;

	address = 0x1;
	status = flash_int_to_address (address, 4, bytes);
	CuAssertIntEquals (test, 0, status);
	CuAssertIntEquals (test, 0x00, bytes[0]);
	CuAssertIntEquals (test, 0x00, bytes[1]);
	CuAssertIntEquals (test, 0x00, bytes[2]);
	CuAssertIntEquals (test, 0x01, bytes[3]);

	address = 0x1002;
	status = flash_int_to_address (address, 4, bytes);
	CuAssertIntEquals (test, 0, status);
	CuAssertIntEquals (test, 0x00, bytes[0]);
	CuAssertIntEquals (test, 0x00, bytes[1]);
	CuAssertIntEquals (test, 0x10, bytes[2]);
	CuAssertIntEquals (test, 0x02, bytes[3]);

	address = 0x341200;
	status = flash_int_to_address (address, 4, bytes);
	CuAssertIntEquals (test, 0, status);
	CuAssertIntEquals (test, 0x00, bytes[0]);
	CuAssertIntEquals (test, 0x34, bytes[1]);
	CuAssertIntEquals (test, 0x12, bytes[2]);
	CuAssertIntEquals (test, 0x00, bytes[3]);

	address = 0x11223344;
	status = flash_int_to_address (address, 4, bytes);
	CuAssertIntEquals (test, 0, status);
	CuAssertIntEquals (test, 0x11, bytes[0]);
	CuAssertIntEquals (test, 0x22, bytes[1]);
	CuAssertIntEquals (test, 0x33, bytes[2]);
	CuAssertIntEquals (test, 0x44, bytes[3]);
}

static void flash_common_test_int_to_address_null (CuTest *test)
{
	uint32_t address;
	int status;

	TEST_START;

	address = 0x1;
	status = flash_int_to_address (address, 3, NULL);
	CuAssertIntEquals (test, FLASH_COMMON_INVALID_ARGUMENT, status);
}

static void flash_common_test_int_to_address_bad_length (CuTest *test)
{
	uint32_t address;
	uint8_t bytes[5];
	int status;

	TEST_START;

	address = 0x1;
	status = flash_int_to_address (address, 2, bytes);
	CuAssertIntEquals (test, FLASH_COMMON_INVALID_ARGUMENT, status);

	status = flash_int_to_address (address, 5, bytes);
	CuAssertIntEquals (test, FLASH_COMMON_INVALID_ARGUMENT, status);
}


CuSuite* get_flash_common_suite ()
{
	CuSuite *suite = CuSuiteNew ();

	SUITE_ADD_TEST (suite, flash_common_test_address_to_int);
	SUITE_ADD_TEST (suite, flash_common_test_address_to_int_4bytes);
	SUITE_ADD_TEST (suite, flash_common_test_address_to_int_null);
	SUITE_ADD_TEST (suite, flash_common_test_address_to_int_bad_length);
	SUITE_ADD_TEST (suite, flash_common_test_int_to_address);
	SUITE_ADD_TEST (suite, flash_common_test_int_to_address_4byte);
	SUITE_ADD_TEST (suite, flash_common_test_int_to_address_null);
	SUITE_ADD_TEST (suite, flash_common_test_int_to_address_bad_length);

	return suite;
}
