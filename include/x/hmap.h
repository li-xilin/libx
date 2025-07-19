/*
 * Copyright (c) 2023,2025 Li Xilin <lixilin@gmx.com>
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

#include "types.h"
#include <stdint.h>

typedef size_t x_hmap_hash_fn(const x_link *node);
typedef bool x_hmap_equal_fn(const x_link *node1, const x_link *node2);

struct x_hmap_st {
	x_list *table;
	size_t load_limit;
	size_t elem_cnt;
	size_t slot_cnt;
	float load_factor;
	uint8_t prime_idx;
	x_hmap_hash_fn *hash;
	x_hmap_equal_fn *equal;
};

uint32_t x_hmap_hash(unsigned key);
void x_hmap_init(x_hmap *ht, float load_factor, x_hmap_hash_fn *hash_fn, x_hmap_equal_fn *equal_fn);
x_link *x_hmap_find(x_hmap *ht, x_link *node);
x_link *x_hmap_find_or_insert(x_hmap *ht, x_link *node);
x_link *x_hmap_insert_or_replace(x_hmap *ht, x_link *node);
x_link *x_hmap_remove(x_hmap *ht, x_link *node);
void x_hmap_free(x_hmap *ht);

#endif
