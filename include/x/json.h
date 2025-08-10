/*
  Copyright (c) 2009-2017 Dave Gamble and x_json contributors
  Copyright (c) 2025 Li Xilin <lixilin@gmx.com>

  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in
  all copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
  THE SOFTWARE.
*/

/* Based on cJSON 1.7.18 */

#ifndef X_JSON_H
#define X_JSON_H

#ifdef __cplusplus
extern "C"
{
#endif

#include "types.h"
#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

#define X_JSON_INVALID 0
#define X_JSON_BOOL    (1 << 0)
#define X_JSON_NULL    (1 << 1)
#define X_JSON_INTEGER (1 << 2)
#define X_JSON_NUMBER  (1 << 3)
#define X_JSON_STRING  (1 << 4)
#define X_JSON_ARRAY   (1 << 5)
#define X_JSON_OBJECT  (1 << 6)
#define X_JSON_RAW     (1 << 7)
#define X_JSON_IS_REF  256
#define X_JSON_STRING_IS_CONST 512

struct x_json_st
{
    struct x_json_st *next;
    struct x_json_st *prev;
    struct x_json_st *child;
    int type;
    char *string;

	union {
		char *string;
		int integer;
		double number;
	} value;
};

#ifndef X_JSON_NESTING_LIMIT
#define X_JSON_NESTING_LIMIT 1000
#endif

#ifndef X_JSON_CIRCULAR_LIMIT
#define X_JSON_CIRCULAR_LIMIT 10000
#endif

const char *x_json_get_error_ptr(void);

x_json *x_json_parse(const char *value);
x_json *x_json_parse2(const char *value, size_t buffer_length);
x_json *x_json_parse3(const char *value, const char **return_parse_end, bool require_null_terminated);
x_json *x_json_parse4(const char *value, size_t buffer_length, const char **return_parse_end, bool require_null_terminated);

char *x_json_print(const x_json *item, bool fmt);
char *x_json_print_buffered(const x_json *item, int prebuffer, bool fmt);
bool x_json_print_to_buffer(x_json *item, char *buffer, const int length, const bool format);
void x_json_free(x_json *item);

int x_json_array_size(const x_json *array);
x_json *x_json_array_at(const x_json *array, int index);
x_json *x_json_object_at(const x_json *const object, const char *const string, bool case_sensitive);
bool x_json_object_exist(const x_json *object, const char *string, bool case_sensitive);

bool x_json_is_invalid(const x_json *const item);
bool x_json_is_false(const x_json *const item);
bool x_json_is_true(const x_json *const item);
bool x_json_is_bool(const x_json *const item);
bool x_json_is_null(const x_json *const item);
bool x_json_is_int(const x_json *const item);
bool x_json_is_number(const x_json *const item);
bool x_json_is_string(const x_json *const item);
bool x_json_is_array(const x_json *const item);
bool x_json_is_object(const x_json *const item);
bool x_json_is_raw(const x_json *const item);
char *x_json_string(const x_json *const item);
double x_json_number(const x_json *const item);

x_json *x_json_create_null(void);
x_json *x_json_create_bool(bool boolean);
x_json *x_json_create_int(double num);
x_json *x_json_create_number(double num);
x_json *x_json_create_string(const char *string);
x_json *x_json_create_raw(const char *raw);
x_json *x_json_create_array(void);
x_json *x_json_create_object(void);

/* Create a string where value.string references a string so
 * it will not be freed by x_json_free */
x_json *x_json_create_string_reference(const char *string);
/* Create an object/array that only references it's elements so
 * they will not be freed by x_json_free */
x_json *x_json_create_object_reference(const x_json *child);
x_json *x_json_create_array_reference(const x_json *child);

x_json *x_json_create_int_array(const int *numbers, int count);
x_json *x_json_create_float_array(const float *numbers, int count);
x_json *x_json_create_double_array(const double *numbers, int count);
x_json *x_json_create_string_array(const char *const *strings, int count);

void x_json_replace_child(x_json *const parent, x_json *const item, x_json *replacement);
x_json *x_json_detach_child(x_json *parent, x_json *const item);

bool x_json_array_add(x_json *array, x_json *item);
bool x_json_object_add(x_json *object, const char *string, bool case_sensitive, x_json *item);
bool x_json_array_addref(x_json *array, x_json *item);
bool x_json_object_addref(x_json *object, const char *string, x_json *item);

x_json *x_json_array_detach(x_json *array, int which);
x_json *x_json_object_detach(x_json *object, const char *string, bool case_sensitive);
void x_json_array_del(x_json *array, int which);
void x_json_object_del(x_json *object, const char *string, bool case_sensitive);

bool x_json_array_insert(x_json *array, int which, x_json *newitem); /* Shifts pre-existing items to the right. */
void x_json_array_replace(x_json *array, int which, x_json *newitem);
void x_json_object_replace(x_json *object, const char *string, bool case_sensitive, x_json *newitem);

void x_json_set_int(x_json *const object, int value);
void x_json_set_float(x_json *const object, double value);
void x_json_set_bool(x_json *const object, bool value);
void x_json_set_string(x_json *object, const char *value);

x_json *x_json_copy(const x_json *item, bool recurse);
/* Duplicate will create a new, identical x_json item to the one you pass, in new memory that will
 * need to be released. With recurse!=0, it will duplicate any children connected to the item.
 * The item->next and ->prev pointers are always zero on return from Duplicate. */
/* Recursively compare two x_json items for equality. If either a or b is NULL or invalid, they will be considered unequal.
 * case_sensitive determines if object keys are treated case sensitive (1) or case insensitive (0) */
bool x_json_compare(const x_json *const a, const x_json *const b, const bool case_sensitive);

/* Minify a strings, remove blank characters(such as ' ', '\t', '\r', '\n') from strings in-placed */
void x_json_minify(char *json);

/* Helper functions for creating and adding items to an object at the same time. */
x_json* x_json_object_add_null(x_json *const object, const char *const name);
x_json* x_json_object_add_true(x_json *const object, const char *const name);
x_json* x_json_object_add_false(x_json *const object, const char *const name);
x_json* x_json_object_add_bool(x_json *const object, const char *const name, const bool boolean);
x_json* x_json_object_add_number(x_json *const object, const char *const name, const double number);
x_json* x_json_object_add_string(x_json *const object, const char *const name, const char *const string);
x_json* x_json_object_add_raw(x_json *const object, const char *const name, const char *const raw);
x_json* x_json_object_add_object(x_json *const object, const char *const name);
x_json* x_json_object_add_array(x_json *const object, const char *const name);

/* Macro for iterating over an array or object */
#define x_json_array_foreach(element, array) \
	for(element = (array != NULL) ? (array)->child : NULL; element != NULL; element = element->next)

#define x_json_object_foreach(element, object) \
	for(element = (object != NULL) ? (object)->child : NULL; element != NULL; element = element->next)

#ifdef __cplusplus
}
#endif

#endif

