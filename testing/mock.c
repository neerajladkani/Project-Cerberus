// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT license.

#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdbool.h>
#include "platform.h"
#include "platform_io.h"
#include "mock.h"
#include "testing.h"


/**
 * Free a single expected call instance.
 *
 * @param call The instance to free.
 */
static void mock_free_call (struct mock_call *call)
{
	int i;

	if (call->argv != NULL) {
		for (i = 0; i < call->argc; i++) {
			if (call->argv[i].ptr_value != NULL) {
				platform_free (call->argv[i].ptr_value);
			}

			if (call->argv[i].flags & MOCK_ARG_FLAG_ALLOCATED) {
				platform_free ((void*) call->argv[i].value);
			}

			if (call->argv[i].flags & MOCK_ARG_FLAG_OUT_ALLOCATED) {
				platform_free ((void*) call->argv[i].out_data);
			}
		}

		platform_free (call->argv);
	}

	platform_free (call);
}

/**
 * Add an expectation to the mock.
 *
 * @param mock The mock instance to add the expectation to.
 * @param func_call The function call that is expected.
 * @param instance The instance the call will be executed against.
 * @param return_val The value to return for the expectation.
 * @param ... A list of expectations for all the arguments to the expected call.
 *
 * @return 0 if the expectation was successfully added or an error code.
 */
int mock_expect (struct mock *mock, void *func_call, void *instance, intptr_t return_val, ...)
{
	struct mock_call *expectation;
	struct mock_expect_arg arg;
	va_list args;
	int i;

	if ((mock == NULL) || (func_call == NULL)) {
		return MOCK_INVALID_ARGUMENT;
	}

	expectation = platform_malloc (sizeof (struct mock_call));
	if (expectation == NULL) {
		return MOCK_NO_MEMORY;
	}

	memset (expectation, 0, sizeof (struct mock_call));

	expectation->func = func_call;
	expectation->instance = instance;
	expectation->return_val = return_val;
	expectation->argc = mock->func_arg_count (func_call);

	if (expectation->argc) {
		expectation->argv = platform_calloc (expectation->argc, sizeof (struct mock_arg));
		if (expectation->argv == NULL) {
			platform_free (expectation);
			return MOCK_NO_MEMORY;
		}
	}

	va_start (args, return_val);
	for (i = 0; i < expectation->argc; i++) {
		arg = va_arg (args, struct mock_expect_arg);

		if (arg.flags & MOCK_ARG_FLAG_ALLOCATED) {
			expectation->argv[i].value = (intptr_t) (platform_malloc (arg.ptr_value_len));
			if (expectation->argv[i].value == 0) {
				mock_free_call (expectation);
				return MOCK_NO_MEMORY;
			}

			memcpy ((void*) expectation->argv[i].value, (void*) arg.value, arg.ptr_value_len);
		}
		else {
			expectation->argv[i].value = arg.value;
		}
		expectation->argv[i].ptr_value_len = arg.ptr_value_len;
		expectation->argv[i].validate = arg.validate;
		expectation->argv[i].flags = arg.flags;
		if (arg.flags & MOCK_ARG_FLAG_SAVED_VALUE) {
			expectation->argv[i].save_arg = arg.save_arg;
		}
		else {
			expectation->argv[i].save_arg = -1;
		}
	}
	va_end (args);

	if (mock->expected == NULL) {
		mock->expected = expectation;
		mock->next_call = expectation;
	}
	else {
		mock->exp_tail->next = expectation;
		if (mock->next_call == NULL) {
			mock->next_call = expectation;
		}
	}
	mock->exp_tail = expectation;
	mock->exp_count++;

	return 0;
}

/**
 * Indicate that a parameter to a mock expectation should be treated as an output parameter.  This
 * will only update the last expectation that was added to the mock.
 *
 * @param mock The mock instance to update.
 * @param arg The argument index to set as an output parameter.  This is a zero-based index based
 * on the variable argument list in the expectation.
 * @param out_data The data to use to fill the parameter when called.  This is stored by reference.
 * @param out_length The length of the output data provided.
 * @param length_arg If the output size is dynamic, this is the argument index that specifies the
 * size of the output buffer provided to the function.  Set this to -1 if the length is fixed.
 *
 * @return 0 if the mock was updated successfully or an error code.
 */
int mock_expect_output (struct mock *mock, int arg, const void *out_data, size_t out_length,
	int length_arg)
{
	if ((mock == NULL) || (out_data == NULL) || (arg < 0)) {
		return MOCK_INVALID_ARGUMENT;
	}

	if (mock->exp_tail == NULL) {
		return MOCK_NO_EXPECTATION;
	}

	if ((arg >= mock->exp_tail->argc) || (length_arg >= mock->exp_tail->argc)) {
		return MOCK_BAD_ARG_INDEX;
	}

	mock->exp_tail->argv[arg].out_data = out_data;
	mock->exp_tail->argv[arg].out_len = out_length;
	mock->exp_tail->argv[arg].size_arg = length_arg;

	return 0;
}

/**
 * Indicate that a parameter to a mock expectation should be treated as an output parameter.  This
 * will only update the last expectation that was added to the mock.
 *
 * This call allows temporary variables to be used for output data.
 *
 * @param mock The mock instance to update.
 * @param arg The argument index to set as an output parameter.  This is a zero-based index based
 * on the variable argument list in the expectation.
 * @param out_data The data to use to fill the parameter when called.  This data is copied.
 * @param out_length The length of the output data provided.
 * @param length_arg If the output size is dynamic, this is the argument index that specifies the
 * size of the output buffer provided to the function.  Set this to -1 if the length is fixed.
 *
 * @return 0 if the mock was updated successfully or an error code.
 */
int mock_expect_output_tmp (struct mock *mock, int arg, const void *out_data, size_t out_length,
	int length_arg)
{
	if ((mock == NULL) || (out_data == NULL) || (arg < 0)) {
		return MOCK_INVALID_ARGUMENT;
	}

	if (mock->exp_tail == NULL) {
		return MOCK_NO_EXPECTATION;
	}

	if ((arg >= mock->exp_tail->argc) || (length_arg >= mock->exp_tail->argc)) {
		return MOCK_BAD_ARG_INDEX;
	}

	mock->exp_tail->argv[arg].out_data = platform_malloc (out_length);
	if (mock->exp_tail->argv[arg].out_data == NULL) {
		return MOCK_NO_MEMORY;
	}

	memcpy ((void*) mock->exp_tail->argv[arg].out_data, out_data, out_length);
	mock->exp_tail->argv[arg].out_len = out_length;
	mock->exp_tail->argv[arg].size_arg = length_arg;
	mock->exp_tail->argv[arg].flags |= MOCK_ARG_FLAG_OUT_ALLOCATED;

	return 0;
}

/**
 * Find the entry for a saved argument with the specified ID.
 *
 * @param mock The mock instance to search.
 * @param id The ID of the saved argument to find.
 *
 * @return The entry for the saved argument or null if no entry with the ID exists.
 */
static struct mock_save_arg* mock_find_save_arg (struct mock *mock, int id)
{
	struct mock_save_arg *pos = mock->save;

	while (pos && (pos->id != id)) {
		pos = pos->next;
	}

	return pos;
}

/**
 * Indicate that the value of a parameter to a mock expectation should be saved upon call.  This
 * saved value can be used as the expected value is future expectations.  This will only update the
 * last expectation that was added to the mock.
 *
 * @param mock The mock instance to update.
 * @param arg The argument index that should be saved.  This is a zero-based index based on the
 * variable argument list in the expectation.
 * @param id The ID to associate with this saved value.  This ID must be unique and will be used in
 * future expectations to reference this saved value.
 *
 * @return 0 if the mock was updated successfully or an error code.
 */
int mock_expect_save_arg (struct mock *mock, int arg, int id)
{
	struct mock_save_arg *saved;

	if ((mock == NULL) || (arg < 0) || (id < 0)) {
		return MOCK_INVALID_ARGUMENT;
	}

	if (mock->exp_tail == NULL) {
		return MOCK_NO_EXPECTATION;
	}

	if (arg >= mock->exp_tail->argc) {
		return MOCK_BAD_ARG_INDEX;
	}

	saved = mock_find_save_arg (mock, id);
	if (saved != NULL) {
		return MOCK_SAVE_ARG_EXISTS;
	}

	saved = platform_malloc (sizeof (struct mock_save_arg));
	if (saved == NULL) {
		return MOCK_NO_MEMORY;
	}

	memset (saved, 0, sizeof (struct mock_save_arg));

	saved->next = mock->save;
	saved->id = id;
	mock->save = saved;
	mock->next_id = id + 1;

	mock->exp_tail->argv[arg].save_arg = id;

	return 0;
}

/**
 * Determine the next available ID for use when created saved arguments.
 *
 * @param mock The mock instance to query.
 *
 * @return The next available ID for a saved value.
 */
int mock_expect_next_save_id (struct mock *mock)
{
	if (mock == NULL) {
		return 0;
	}

	return mock->next_id;
}

/**
 * Share a saved argument from one mock instance with another.  A saved argument that is shared will
 * be saved in both mock instances when the expectation that saves the argument is called.  It will
 * exist independently in the context of both mock instances for validation.
 *
 * @param from The mock instance that contains the expectation that will save the argument value.
 * @param src_id The ID for the argument to share.
 * @param to The mock instance that will be shared the saved argument.
 * @param dest_id The ID to assign the argument in the shared mock instance.
 *
 * @return 0 if the argument was shared successfully or an error code.
 */
int mock_expect_share_save_arg (struct mock *from, int src_id, struct mock *to, int dest_id)
{
	struct mock_save_arg *saved;
	struct mock_save_arg *shared;

	if ((from == NULL) || (to == NULL) || (src_id < 0) || (dest_id < 0)) {
		return MOCK_INVALID_ARGUMENT;
	}

	saved = mock_find_save_arg (from, src_id);
	if (saved == NULL) {
		return MOCK_NO_SAVE_ARG;
	}

	shared = platform_malloc (sizeof (struct mock_save_arg));
	if (shared == NULL) {
		return MOCK_NO_MEMORY;
	}

	memset (shared, 0, sizeof (struct mock_save_arg));

	shared->next = to->save;
	shared->id = dest_id;
	to->save = shared;

	saved->shared = shared;

	return 0;
}

/**
 * Print the information for an expected function call.
 *
 * @param mock The mock expecting the function call.
 * @param call The call information to print.
 */
static void mock_print_func (struct mock *mock, struct mock_call *call)
{
	int i;
	platform_printf ("%s (%p", mock->func_name_map (call->func), call->instance);
	for (i = 0; i < call->argc; i++) {
		platform_printf (", 0x%lx", call->argv[i].value);
	}
	platform_printf (")" NEWLINE);
}

/**
 * Validate a received function argument against an expected one.
 *
 * @param mock The mock instance being verified.
 * @param cur_exp The index of the current expectation being checked.
 * @param arg_name The name of the argument being validated.
 * @param expected The expected argument.
 * @param actual The actual argument passed to the function.
 *
 * @return 0 if the argument matched the expected or 1 if not.
 */
static int mock_validate_arg (struct mock *mock, int cur_exp, const char *arg_name,
	struct mock_arg *expected, struct mock_arg *actual)
{
	int fail = 0;

	if (!(expected->flags & MOCK_ARG_FLAG_ANY_VALUE)) {
		if (expected->flags & MOCK_ARG_FLAG_NOT_NULL) {
			if (actual->value == 0) {
				if (!(expected->flags & MOCK_ARG_FLAG_PTR_PTR) ||
					(actual->flags & MOCK_ARG_FLAG_PTR_PTR)) {
					platform_printf ("(%s, %d) Unexpected NULL argument: name=%s" NEWLINE,
						mock->name, cur_exp, arg_name);
				}
				else {
					platform_printf ("(%s, %d) Unexpected NULL pointer to pointer: name=%s" NEWLINE,
						mock->name, cur_exp, arg_name);
				}
				fail = 1;
			}

			if (expected->ptr_value_len) {
				if (actual->ptr_value == NULL) {
					platform_printf ("(%s, %d) No pointer contents to validate: name=%s" NEWLINE,
						mock->name, cur_exp, arg_name);
					fail = 1;
				}
				else {
					char prefix[100];

					snprintf (prefix, sizeof (prefix), "(%s, %d, arg=%s) ", mock->name, cur_exp,
						arg_name);

					if (expected->validate) {
						fail |= expected->validate (prefix, (void*) expected->value,
							actual->ptr_value);
					}
					else {
						fail |= testing_validate_array_prefix ((void*) expected->value,
							actual->ptr_value, expected->ptr_value_len, prefix);
					}
				}
			}
		}
		else if (expected->flags & MOCK_ARG_FLAG_SAVED_VALUE) {
			struct mock_save_arg *saved = mock_find_save_arg (mock, expected->save_arg);
			if (saved) {
				if (saved->saved) {
					if (saved->value != actual->value) {
						platform_printf ("(%s, %d) Unexpected saved argument: id=%d, name=%s, expected=0x%lx, actual=0x%lx"
							NEWLINE, mock->name, cur_exp, expected->save_arg, arg_name,
							saved->value, actual->value);
						fail = 1;
					}
				}
				else {
					platform_printf ("(%s, %d) Argument ID %d value not saved." NEWLINE, mock->name,
						cur_exp, expected->save_arg);
					fail = 1;
				}
			}
			else {
				platform_printf ("(%s, %d) Unknown saved argument ID: id=%d" NEWLINE, mock->name,
					cur_exp, expected->save_arg);
				fail = 1;
			}
		}
		else if (expected->value != actual->value) {
			platform_printf ("(%s, %d) Unexpected argument: name=%s, expected=0x%lx, actual=0x%lx"
				NEWLINE, mock->name, cur_exp, arg_name, expected->value, actual->value);
			fail = 1;
		}
	}

	return fail;
}

/**
 * Verify that all expected calls were executed.
 *
 * @param mock The mock to verify.
 *
 * @return 0 if the expectations were all met or 1 if not.
 */
int mock_validate (struct mock *mock)
{
	int fail = 0;
	struct mock_call *exp_pos;
	struct mock_call *call_pos;
	struct mock_call *search;
	int current = 0;
	int i;

	if (mock) {
		if (mock->exp_count != mock->call_count) {
			platform_printf ("(%s) Unexpected number of function calls: expected=%d, actual=%d"
				NEWLINE, mock->name, mock->exp_count, mock->call_count);
			fail = 1;
		}

		exp_pos = mock->expected;
		call_pos = mock->called;

		while ((exp_pos != NULL) || (call_pos != NULL)) {
			if (call_pos == NULL) {
				platform_printf ("(%s, %d) Not called: ", mock->name, current);
				mock_print_func (mock, exp_pos);
				fail = 1;

				current++;
				exp_pos = exp_pos->next;
			}
			else if (exp_pos == NULL) {
				platform_printf ("(%s, after %d) Unexpected call: ", mock->name, current);
				mock_print_func (mock, call_pos);
				fail = 1;

				call_pos = call_pos->next;
			}
			else {
				if (exp_pos->func != call_pos->func) {
					search = exp_pos->next;
					while ((search != NULL) && (call_pos->func != search->func)) {
						search = search->next;
					}

					if (search != NULL) {
						while (exp_pos != search) {
							platform_printf ("(%s, %d) Not called: ", mock->name, current);
							mock_print_func (mock, exp_pos);
							fail = 1;

							current++;
							exp_pos = exp_pos->next;
						}
					}
					else {
						platform_printf ("(%s, before %d) Unexpected call: ", mock->name, current);
						mock_print_func (mock, call_pos);
						fail = 1;

						call_pos = call_pos->next;
					}
				}

				if ((exp_pos != NULL) && (call_pos != NULL) && (exp_pos->func == call_pos->func)) {
					if (exp_pos->instance != call_pos->instance) {
						platform_printf ("(%s, %d) Unexpected object instance: expected=%p, actual=%p"
							NEWLINE, mock->name, current, exp_pos->instance, call_pos->instance);
						fail = 1;
					}

					if (exp_pos->argc != call_pos->argc) {
						platform_printf ("(%s, %d) Unexpected number of arguments: expected=%d, actual=%d"
							NEWLINE, mock->name, current, exp_pos->argc, call_pos->argc);
						fail = 1;
					}
					else {
						for (i = 0; i < exp_pos->argc; i++) {
							fail |= mock_validate_arg (mock, current,
								mock->arg_name_map (exp_pos->func, i), &exp_pos->argv[i],
								&call_pos->argv[i]);
						}
					}

					current++;
					exp_pos = exp_pos->next;
					call_pos = call_pos->next;
				}
			}
		}
	}
	else {
		fail = 1;
	}

	return fail;
}


/**
 * Initialize the mock instance.
 *
 * @param mock The mock to initialize.
 *
 * @return 0 if the mock was successfully initialized or an error code.
 */
int mock_init (struct mock *mock)
{
	if (mock == NULL) {
		return MOCK_INVALID_ARGUMENT;
	}

	memset (mock, 0, sizeof (struct mock));
	mock->name = "mock";

	return 0;
}

/**
 * Release all memory used by a list of call entries.
 *
 * @param head The head of the list of calls.
 */
static void mock_release_call_list (struct mock_call *head)
{
	struct mock_call *next;

	while (head != NULL) {
		next = head->next;
		mock_free_call (head);
		head = next;
	}
}

/**
 * Release all memory used by a list of saved arguments.
 *
 * @param head The head of the list saved arguments.
 */
static void mock_release_saved_args (struct mock_save_arg *head)
{
	struct mock_save_arg *next;

	while (head != NULL) {
		next = head->next;
		platform_free (head);
		head = next;
	}
}

/**
 * Release the resources used by a mock instance.
 *
 * @param mock The mock instance to release.
 */
void mock_release (struct mock *mock)
{
	if (mock != NULL) {
		mock_release_call_list (mock->expected);
		mock_release_call_list (mock->called);
		mock_release_saved_args (mock->save);
	}
}

/**
 * Set the name for the mock instance.
 *
 * @param mock The mock to update.
 * @param name The name to assign to the mock.
 */
void mock_set_name (struct mock *mock, const char *name)
{
	if (mock != NULL) {
		mock->name = (name != NULL) ? name : "mock";
	}
}

/**
 * Allocate a instance for an executed function call.
 *
 * @param func The function that was called.
 * @param instance The instance the function was called against.
 * @param args The number of arguments passed to the function.
 * @param ... A list of intptr_t arguments from the function.
 *
 * @return The new call instance or null.
 */
struct mock_call* mock_allocate_call (void *func, void *instance, size_t args, ...)
{
	struct mock_call *call;
	va_list arg_list;
	int i;

	call = platform_malloc (sizeof (struct mock_call));
	if (call == NULL) {
		return NULL;
	}

	memset (call, 0, sizeof (struct mock_call));

	if (args) {
		call->argv = platform_calloc (args, sizeof (struct mock_arg));
		if (call->argv == NULL) {
			platform_free (call);
			return NULL;
		}
	}

	call->func = func;
	call->instance = instance;
	call->argc = args;

	va_start (arg_list, args);
	for (i = 0; i < args; i++) {
		call->argv[i].value = va_arg (arg_list, intptr_t);
	}
	va_end (arg_list);

	return call;
}

/**
 * Push an executed function call on to the call list.
 *
 * @param mock The mock that executed the call.
 * @param call the call that was executed.
 */
static void mock_push_call (struct mock *mock, struct mock_call *call)
{
	if (mock->called == NULL) {
		mock->called = call;
	}
	else {
		mock->call_tail->next = call;
	}

	mock->call_tail = call;
	mock->call_count++;
}

/**
 * Return from an executed function call.
 *
 * @param mock The mock that executed the call.
 * @param call The call that was executed.
 *
 * @return The result of the function execution.
 */
intptr_t mock_return_from_call (struct mock *mock, struct mock_call *call)
{
	struct mock_call *expected;
	intptr_t status = 0;
	int i;
	size_t out_len;

	if (call == NULL) {
		return MOCK_NO_MEMORY;
	}

	mock_push_call (mock, call);

	expected = mock->next_call;
	while ((expected != NULL) && (expected->func != call->func)) {
		expected = expected->next;
	}

	if (expected != NULL) {
		for (i = 0; i < expected->argc; i++) {
			/* Dereference pointer parameters. */
			if ((call->argv[i].value != 0) && (expected->argv[i].flags & MOCK_ARG_FLAG_PTR_PTR)) {
				call->argv[i].value = (intptr_t) (*((int**) call->argv[i].value));
				call->argv[i].flags |= MOCK_ARG_FLAG_PTR_PTR;
			}

			/* Save the contents of a pointer parameter. */
			if ((expected->argv[i].ptr_value_len != 0) && (call->argv[i].value != 0)) {
				call->argv[i].ptr_value = platform_malloc (expected->argv[i].ptr_value_len);

				if (call->argv[i].ptr_value != NULL) {
					call->argv[i].ptr_value_len = expected->argv[i].ptr_value_len;
					memcpy (call->argv[i].ptr_value, (void*) call->argv[i].value,
						call->argv[i].ptr_value_len);
				}
			}

			/* Fill an output parameter with data. */
			if ((expected->argv[i].out_data != NULL) && (call->argv[i].value != 0)) {
				if (expected->argv[i].size_arg < 0) {
					out_len = expected->argv[i].out_len;
				}
				else if (expected->argv[i].out_len > call->argv[expected->argv[i].size_arg].value) {
					out_len = call->argv[expected->argv[i].size_arg].value;
				}
				else {
					out_len = expected->argv[i].out_len;
				}

				memcpy ((void*) call->argv[i].value, expected->argv[i].out_data, out_len);
			}

			/* Save argument values. */
			if (expected->argv[i].save_arg >= 0) {
				struct mock_save_arg *save = mock_find_save_arg (mock, expected->argv[i].save_arg);
				if (save && !save->saved) {
					save->value = call->argv[i].value;
					save->saved = true;

					if (save->shared) {
						save->shared->value = save->value;
						save->shared->saved = true;
					}
				}
			}
		}

		status = expected->return_val;
		mock->next_call = expected->next;
	}

	return status;
}
