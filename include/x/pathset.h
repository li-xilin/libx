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
#ifndef X_PATHSET_H
#define X_PATHSET_H

#include "x/types.h"
#include "x/list.h"
#include "x/rwlock.h"
#include "x/memory.h"
#include "x/uchar.h"

struct x_pathset_st
{
	x_mset mset;
	uint32_t unitary_mask;
	x_list dir_list;
	x_rwlock lock;
};

struct x_pathset_entry_st
{
	x_link link;
	uint16_t path_len;
	x_uchar path[];
};

void x_pathset_init(x_pathset *pset);
void x_pathset_free(x_pathset *pset);
void x_pathset_clear(x_pathset *pset);
uint32_t x_pathset_mask(x_pathset *pset, const x_uchar *path);
uint32_t x_pathset_insert(x_pathset *pset, uint32_t mask, const x_uchar *path, bool is_leaf);
uint32_t x_pathset_remove(x_pathset *pset, uint32_t mask, const x_uchar *path, bool is_leaf);
uint32_t x_pathset_unitary_mask(x_pathset *pset);
void x_pathset_dump(x_pathset *pset, x_strbuf *strbuf);
void x_pathset_find_top(x_pathset *pset, x_list *list);
void x_pathset_free_entry_list(x_list *list);

#endif


