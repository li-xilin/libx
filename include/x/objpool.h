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
#ifndef X_MEMCACHE_H
#define X_MEMCACHE_H

#include "linkage.h"
#include "types.h"
#include "list.h"

X_EXTERN_C_BEGIN

#define X_OBJPOOL_INIT(_item_size) { _item_size }

struct x_slab_st {
	x_link link;
	uint64_t slots;
	uintptr_t refcnt_or_head;
	uint8_t data[];
};

struct x_objpool_st {
	size_t item_size;
	size_t slab_size;
	size_t block_size;
	uint64_t empty_slotmask;
	x_list partial, empty, full;
};

void x_objpool_init(x_objpool *pool, size_t itemsize);
void x_objpool_free(x_objpool *pool);
void *x_objpool_alloc(x_objpool *pool);
void x_objpool_dealloc(x_objpool *pool, void *ptr);
void x_objpool_show(const x_objpool *pool, void *fp);
bool x_objpool_sanitize(const x_objpool *pool);

X_EXTERN_C_END

#endif

