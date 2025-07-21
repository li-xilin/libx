/*
 * Copyright (c) 2022 Armon Dadgar
 * Copyright (c) 2024,2025 Li Xilin <lixilin@gmx.com>
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

#ifndef X_HEAP_H
#define X_HEAP_H

#include "types.h"

struct x_ranode_st
{
	size_t index;
};

typedef bool x_heap_ordered_fn(const x_ranode *, const x_ranode *);

struct x_heap_st
{
	x_heap_ordered_fn *cmp;
	size_t entry_cnt;
	size_t min_page_cnt;
	size_t page_cnt;
	x_ranode **table;
};

void x_heap_init(x_heap *h, x_heap_ordered_fn *cmp);
void x_heap_free(x_heap *h);
void x_heap_push(x_heap *h, x_ranode *n);
x_ranode *x_heap_top(const x_heap *h);
x_ranode *x_heap_pop(x_heap *h);
void x_heap_remove(x_heap *h, x_ranode *n);

#endif

