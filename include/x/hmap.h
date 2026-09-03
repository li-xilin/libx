/*
 * Copyright (c) 2022-2026 Li Xilin <lixilin@gmx.com>
 * 
 * Permission is hereby granted, free of charge, to one person obtaining a copy
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

#ifndef X_HMAP_H
#define X_HMAP_H

#include "list.h"
#include "types.h"
#include "memory.h"
#include "flowctl.h"
#include <stdint.h>

#define X_HMAP_INIT(factor, equal_fn) { \
	.load_factor = factor, \
	.equal = equal_fn, \
}

#define x_hmap_foreach(hnode, hmap) \
	for (size_t __x_i = 0; __x_i < (hmap)->slot_cnt; __x_i++) \
		x_slist_foreach_prev(__x_prev, &(hmap)->table[__x_i]) \
			x_block_var(x_hnode *hnode = x_container_of(__x_prev->next, x_hnode, link))

typedef bool x_hmap_equal_fn(const x_hnode *a, const x_hnode *b);

struct x_hmap_st {
	x_slist *table;
	size_t elem_cnt;
	size_t slot_cnt;
	float load_factor;
	uint8_t prime_index;
	x_hmap_equal_fn *equal;
};

struct x_hnode_st {
	x_slink link;
	size_t hash;
};

void x_hmap_init(x_hmap *ht, float load_factor, x_hmap_equal_fn *equal_fn);
void x_hmap_free(x_hmap *ht);
x_hnode *x_hmap_find(const x_hmap *ht, const x_hnode *node);
x_hnode *x_hmap_find_or_insert(x_hmap *ht, x_hnode *node);
x_hnode *x_hmap_insert_or_replace(x_hmap *ht, x_hnode *node);
x_hnode *x_hmap_find_and_remove(x_hmap *ht, const x_hnode *node);
void x_hmap_remove(x_hmap *ht, const x_hnode *node);
void x_hmap_clear(x_hmap *ht);
bool x_hmap_sanitize(x_hmap *ht);

#endif

