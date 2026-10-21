/*
 * Copyright (c) 2026 Li Xilin <lixilin@gmx.com>
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */
#ifndef X_ARRAY_H
#define X_ARRAY_H

#include "x/types.h"
#include "x/assert.h"
#include "x/memory.h"
#include <string.h>

#define X_ARRAY_MIN_SIZE 8

#define X_ARRAY_INIT(elem_size) { (elem_size) }
#define X_ARRAY_AT(t, a, i) *((t *)x_array_at(a, i))
#define x_array_foreach(type, var, arr) \
	for (type *var = (void *)(arr)->data, *__x_end = (type *)((arr)->data + (arr)->size * (arr)->elem_size);  \
			var != __x_end; var = (type *)((char *)var + (arr)->elem_size))

struct x_array_st
{
	size_t elem_size;
	size_t capacity;
	size_t size;
	uint8_t *data;
};

static inline void x_array_init(x_array *arr, size_t elem_size)
{
	arr->elem_size = elem_size;
	arr->capacity = 0;
	arr->data = NULL;
	arr->size = 0;
}

static inline void x_array_free(x_array *array)
{
	x_assert(array != NULL);
	x_free(array->data);
}

static inline void x_array_reserve(x_array *array, size_t new_capacity)
{
	x_assert(array != NULL);
	if (new_capacity < X_ARRAY_MIN_SIZE)
		new_capacity = X_ARRAY_MIN_SIZE;
	void *new_data = x_realloc(array->data, new_capacity * array->elem_size);
	array->data = new_data;
	array->capacity = new_capacity;
}

static inline size_t x_array_size(x_array *array)
{
	x_assert(array != NULL);
	return array->size;
}

static inline size_t x_array_capacity(x_array *array)
{
	x_assert(array != NULL);
	return array->capacity;
}

static inline void *x_array_data(x_array *array)
{
	x_assert(array != NULL);
	return array->data;
}

static inline bool x_array_is_empty(x_array *array)
{
	x_assert(array != NULL);
	return !x_array_size(array);
}

static inline bool x_array_index_is_valid(x_array *array, int index)
{
	x_assert(array != NULL);
	if (x_array_is_empty(array))
		return false;
	return index >= 0 && index <= array->size;
}

static inline void *x_array_at(x_array *array, int index)
{
	x_assert(array != NULL);
	if (!x_array_index_is_valid(array, index))
		return NULL;
	return array->data + index * array->elem_size;
}

static inline void x_array_insert(x_array *array, int index, void *item)
{
	x_assert(array != NULL);
	x_assert(x_array_index_is_valid(array, index) || index == array->size);
	array->size++;
	if (array->size >= array->capacity)
		x_array_reserve(array, array->capacity << 1);
	if (index != array->size) {
		memmove(array->data + (index + 1) * array->elem_size,
				array->data + index * array->elem_size,
				(array->size - index) * array->elem_size);
	}
	memcpy(array->data + index * array->elem_size, item, array->elem_size);
}

static inline void *x_array_push_back(x_array *array, void *item)
{
	x_assert(array != NULL);
	if (array->size >= array->capacity)
		x_array_reserve(array, array->capacity * 2);
	array->size++;
	void *target = array->data + (array->size - 1) * array->elem_size;
	memcpy(target, item, array->elem_size);
	return target;
}

static inline bool x_array_remove_last(x_array *array)
{
	x_assert(array != NULL);
	if (x_array_is_empty(array))
		return false;
	array->size--;
	if (array->size >= (X_ARRAY_MIN_SIZE << 1) && array->size < (array->capacity >> 2))
		x_array_reserve(array, array->capacity >> 1);
	return true;
}

static inline void *x_array_pop_back(x_array *array)
{
	x_assert(array != NULL);
	if (x_array_is_empty(array))
		return false;
	array->size--;
	return array->data + array->size * array->elem_size;
}

static inline void x_array_shrink(x_array *array)
{
	x_assert(array != NULL);
	size_t new_capacity = array->capacity;
	while (new_capacity >= X_ARRAY_MIN_SIZE * 2 && array->size < (new_capacity >> 2))
		new_capacity >>= 1;
	if (new_capacity != array->capacity)
		x_array_reserve(array, new_capacity);
}

static inline bool x_array_remove_at(x_array *array, int index)
{
	x_assert(array != NULL);
	if (!x_array_index_is_valid(array, index))
		return false;
	if (index >= array->size)
		return false;
	memmove(array->data + index * array->elem_size,
			array->data + (index + 1) * array->elem_size,
			(array->size - index - 1) * array->elem_size);
	x_array_remove_last(array);
	return true;
}

static inline void x_array_clear(x_array *array)
{
	x_assert(array != NULL);
	array->data = x_realloc(array->data, array->elem_size * X_ARRAY_MIN_SIZE); 
	x_array_init(array, array->elem_size);
}

#endif

