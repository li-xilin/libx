/*
 * Copyright (c) 2024-2026 Li Xilin <lixilin@gmx.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#ifndef X_MEMORY_H
#define X_MEMORY_H

#include "linkage.h"
#include "types.h"
#include "list.h"
#include "mutex.h"
#include "detect.h"
#include <stdlib.h>

X_EXTERN_C_BEGIN

#define X_MSET_INIT(mset) { .list = X_LIST_INIT(mset.list), .lock = X_MUTEX_INIT }

#ifndef X_STACK_ALLOC_LIMIT
#define X_STACK_ALLOC_LIMIT 4096
#endif

#if defined(X_CC_GNU) || defined(X_CC_CLANG)
#	define X_ALLOCA(n)   __builtin_alloca(n)
#elif defined(X_CC_MSVC)
#	include <malloc.h>
#	define X_ALLOCA(n)   _alloca(n)
#elif defined(__has_include)
#	if __has_include(<alloca.h>)
#		include <alloca.h>
#		define X_ALLOCA(n)   alloca(n)
#	elif __has_include(<malloc.h>)
#		include <malloc.h>
#		define X_ALLOCA(n)   alloca(n)
#	endif
#endif

static inline void *__x_assign_and_shift(void *p, uintptr_t v)
{
	uintptr_t *tmp =  p;
	tmp[0] = v;
	return tmp + 1;
}

#ifdef X_ALLOCA
#	define x_stack_alloc(size) ((size) > X_STACK_ALLOC_LIMIT \
		?  __x_assign_and_shift(x_malloc_retry((size) + sizeof(uintptr_t)), 1)\
		: (size) > 0 \
			? __x_assign_and_shift(X_ALLOCA((size) + sizeof(uintptr_t)), 0) \
			: NULL)
#	define x_stack_free(ptr) ((ptr) && ((uintptr_t *)(ptr))[-1] ? (void)free((uintptr_t *)(ptr) - 1) : (void)(ptr))
#else
#	define x_stack_alloc(size) x_malloc_retry(size)
#	define x_stack_free(ptr) free(ptr)
#endif

struct x_mblock_st
{
	x_mset *mset;
	x_link link;
	char data[];
};

struct x_mset_st
{
	x_list list;
	x_mutex lock;
};

void *x_malloc_retry(size_t size);
void *x_realloc_retry(void *old_ptr, size_t size);

void x_mset_init(x_mset *mset);
void x_mset_clear(x_mset *mset);
void x_mset_free(x_mset *mset);
void *x_calloc(x_mset *mset, size_t nmemb, size_t size);
void *x_malloc(x_mset *mset, size_t size);
void *x_mcopy(x_mset *mset, const void *ptr, size_t size);
void *x_zalloc(x_mset *mset, size_t size);
void *x_realloc(void *ptr, size_t size);
void x_free(void *ptr);
void x_mdetach(void *ptr);
void x_mattach(x_mset *mset, void *ptr);

void *x_aligned_alloc(size_t align, size_t size);
void *x_aligned_alloc_retry(size_t align, size_t size);
void x_aligned_free(void *ptr);

X_EXTERN_C_END

#endif

