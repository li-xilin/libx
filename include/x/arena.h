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
#ifndef X_ARENA_H
#define X_ARENA_H

#include "linkage.h"
#include "types.h"
#include "list.h"

X_EXTERN_C_BEGIN

#define X_ARENA_PAGE_SIZE 4096
#define X_ARENA_INIT_1(page_size) { X_SLIST_INIT, X_SLIST_INIT, (page_size) }
#define X_ARENA_INIT X_ARENA_INIT_1(X_ARENA_PAGE_SIZE)

struct x_arena_page_st {
	x_slink link;
	size_t current;
	uint8_t region[];
};

struct x_arena_st {
	x_slist pages;
	x_slist large_pages;
	size_t page_size;
};

void x_arena_init1(x_arena *arena, size_t page_size);
void x_arena_init(x_arena *arena);
void *x_arena_alloc(x_arena *arena, size_t size);
void x_arena_free(x_arena *arena);

X_EXTERN_C_END

#endif
