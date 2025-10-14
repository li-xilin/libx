/*
 * Copyright (c) 2025 Li Xilin <lixilin@gmx.com>
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
#ifndef X_INDEX_H
#define X_INDEX_H

#include "x/splay.h"
#include "x/list.h"
#include "x/memory.h"

struct x_indexset_st
{
	x_indexer *indexer;
	x_btnode node;
	x_list entries;
};

struct x_index_st
{
	x_indexset *ilist;
	x_link link;
};

typedef int x_index_cmp_f(const x_index *x, const x_index *y);

struct x_indexer_st
{
	x_index_cmp_f *cmp;
	x_mset mset;
	x_splay indexes;
};

void x_index_init(x_index *idx);
void x_index_insert(x_index *idx, x_indexer *indexer);
void x_index_remove(x_index *idx);

void x_indexer_init(x_indexer *indexer, x_index_cmp_f *cmp);
void x_indexer_free(x_indexer *indexer);
const x_indexset *x_indexer_find(x_indexer *indexer, const x_index *pattern);

x_index *x_indexset_first(const x_indexset *iset);
x_index *x_indexset_next(const x_index *idx);

#define x_indexset_foreach(idx, set) \
	for (x_index *idx = x_indexset_first(iset); idx; idx = x_indexset_next(idx))

#endif

