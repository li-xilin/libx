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
#define X_JSON_FALSE   (1 << 0)
#define X_JSON_TRUE    (1 << 1)
#define x_json_NULL    (1 << 2)
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
    char *valuestring;
    int valueint;
    double valuedouble;
    char *string;
};

typedef struct x_json_hooks
{
      void *(*malloc_fn)(size_t sz);
      void (*free_fn)(void *ptr);
} x_json_hooks;

#ifndef X_JSON_NESTING_LIMIT
#define X_JSON_NESTING_LIMIT 1000
#endif

#ifndef X_JSON_CIRCULAR_LIMIT
#define X_JSON_CIRCULAR_LIMIT 10000
#endif

void x_json_init_hooks(x_json_hooks* hooks);
const char *x_json_get_error_ptr(void);

x_json *x_json_parse(const char *value);
x_json *x_json_parse2(const char *value, size_t buffer_length);
x_json *x_json_parse3(const char *value, const char **return_parse_end, bool require_null_terminated);
x_json *x_json_parse4(const char *value, size_t buffer_length, const char **return_parse_end, bool require_null_terminated);

char *x_json_print(const x_json *item, bool fmt);
char *x_json_print_buffered(const x_json *item, int prebuffer, bool fmt);
bool x_json_print_to_buffer(x_json *item, char *buffer, const int length, const bool format);
void x_json_delete(x_json *item);

int x_json_get_array_size(const x_json *array);
x_json *x_json_get_array_item(const x_json *array, int index);
x_json *x_json_get_object_item(const x_json *const object, const char *const string);
x_json *x_json_get_object_item_case_sensitive(const x_json *const object, const char *const string);
bool x_json_has_object_item(const x_json *object, const char *string);

bool x_json_is_invalid(const x_json *const item);
bool x_json_is_false(const x_json *const item);
bool x_json_is_true(const x_json *const item);
bool x_json_is_bool(const x_json *const item);
bool x_json_is_null(const x_json *const item);
bool x_json_is_number(const x_json *const item);
bool x_json_is_string(const x_json *const item);
bool x_json_is_array(const x_json *const item);
bool x_json_is_object(const x_json *const item);
bool x_json_is_raw(const x_json *const item);
char *x_json_get_string_value(const x_json *const item);
double x_json_get_number_value(const x_json *const item);

x_json *x_json_create_null(void);
x_json *x_json_create_true(void);
x_json *x_json_create_false(void);
x_json *x_json_create_bool(bool boolean);
x_json *x_json_create_number(double num);
x_json *x_json_create_string(const char *string);
x_json *x_json_create_raw(const char *raw);
x_json *x_json_create_array(void);
x_json *x_json_create_object(void);

/* Create a string where valuestring references a string so
 * it will not be freed by x_json_delete */
x_json *x_json_create_string_reference(const char *string);
/* Create an object/array that only references it's elements so
 * they will not be freed by x_json_delete */
x_json *x_json_create_object_reference(const x_json *child);
x_json *x_json_create_array_reference(const x_json *child);

x_json *x_json_create_int_array(const int *numbers, int count);
x_json *x_json_create_float_array(const float *numbers, int count);
x_json *x_json_create_double_array(const double *numbers, int count);
x_json *x_json_create_string_array(const char *const *strings, int count);

bool x_json_add_item_to_array(x_json *array, x_json *item);
bool x_json_add_item_to_object(x_json *object, const char *string, x_json *item);
/* Use this when string is definitely const (i.e. a literal, or as good as), and will definitely survive the x_json object.
 * WARNING: When this function was used, make sure to always check that (item->type & X_JSON_STRING_IS_CONST) is zero before
 * writing to `item->string` */
bool x_json_add_item_to_object_cs(x_json *object, const char *string, x_json *item);
/* Append reference to item to the specified array/object. Use this when you want to add an existing x_json to a new x_json, but don't want to corrupt your existing x_json. */
bool x_json_add_item_reference_to_array(x_json *array, x_json *item);
bool x_json_add_item_reference_to_object(x_json *object, const char *string, x_json *item);

x_json *x_json_detach_item_via_pointer(x_json *parent, x_json *const item);
x_json *x_json_detach_item_from_array(x_json *array, int which);
void x_json_delete_item_from_array(x_json *array, int which);
x_json *x_json_detach_item_from_object(x_json *object, const char *string);
x_json *x_json_detach_item_from_object_case_sensitive(x_json *object, const char *string);
void x_json_delete_item_from_object(x_json *object, const char *string);
void x_json_delete_item_from_object_case_sensitive(x_json *object, const char *string);

bool x_json_insert_item_in_array(x_json *array, int which, x_json *newitem); /* Shifts pre-existing items to the right. */
bool x_json_replace_item_via_pointer(x_json *const parent, x_json *const item, x_json *replacement);
bool x_json_replace_item_in_array(x_json *array, int which, x_json *newitem);
bool x_json_replace_item_in_object(x_json *object,const char *string,x_json *newitem);
bool x_json_replace_item_in_object_case_sensitive(x_json *object,const char *string,x_json *newitem);

x_json *x_json_duplicate(const x_json *item, bool recurse);
/* Duplicate will create a new, identical x_json item to the one you pass, in new memory that will
 * need to be released. With recurse!=0, it will duplicate any children connected to the item.
 * The item->next and ->prev pointers are always zero on return from Duplicate. */
/* Recursively compare two x_json items for equality. If either a or b is NULL or invalid, they will be considered unequal.
 * case_sensitive determines if object keys are treated case sensitive (1) or case insensitive (0) */
bool x_json_compare(const x_json *const a, const x_json *const b, const bool case_sensitive);

/* Minify a strings, remove blank characters(such as ' ', '\t', '\r', '\n') from strings in-placed */
void x_json_minify(char *json);

/* Helper functions for creating and adding items to an object at the same time. */
x_json* x_json_add_null_to_object(x_json *const object, const char *const name);
x_json* x_json_add_true_to_object(x_json *const object, const char *const name);
x_json* x_json_add_false_to_object(x_json *const object, const char *const name);
x_json* x_json_add_bool_to_object(x_json *const object, const char *const name, const bool boolean);
x_json* x_json_add_number_to_object(x_json *const object, const char *const name, const double number);
x_json* x_json_add_string_to_object(x_json *const object, const char *const name, const char *const string);
x_json* x_json_add_raw_to_object(x_json *const object, const char *const name, const char *const raw);
x_json* x_json_add_object_to_object(x_json *const object, const char *const name);
x_json* x_json_add_array_to_object(x_json *const object, const char *const name);

/* When assigning an integer value, it needs to be propagated to valuedouble too. */
#define x_json_set_int_value(object, number) ((object) ? (object)->valueint = (object)->valuedouble = (number) : (number))
/* helper for the x_json_set_number_value macro */
double x_json_set_number_helper(x_json *object, double number);
#define x_json_set_number_value(object, number) ((object != NULL) ? x_json_set_number_helper(object, (double)number) : (number))
/* Change the valuestring of a X_JSON_STRING object, only takes effect when type of object is X_JSON_STRING */
char* x_json_set_valuestring(x_json *object, const char *valuestring);

/* If the object is not a boolean type this does nothing and returns X_JSON_INVALID else it returns the new type*/
#define x_json_set_bool_value(object, boolValue) ( \
    (object != NULL && ((object)->type & (X_JSON_FALSE|X_JSON_TRUE))) ? \
    (object)->type=((object)->type &(~(X_JSON_FALSE|X_JSON_TRUE)))|((boolValue)?X_JSON_TRUE:X_JSON_FALSE) : \
    X_JSON_INVALID\
)

/* Macro for iterating over an array or object */
#define x_json_array_foreach(element, array) \
	for(element = (array != NULL) ? (array)->child : NULL; element != NULL; element = element->next)

void *x_json_malloc(size_t size);
void x_json_free(void *object);

#ifdef __cplusplus
}
#endif

#endif


