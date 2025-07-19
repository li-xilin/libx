/*
 * Copyright (c) 2022-2023,2025 Li Xilin <lixilin@gmx.com>
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

#include "x/hmap.h"
#include "x/list.h"
#include "x/memory.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <assert.h>

static const int s_primes[] =
{
	53, 97, 193, 389, 769, 1543, 3079, 6151,
	12289, 24593, 49157, 98317, 196613, 393241,
	786433, 1572869, 3145739, 3145739, 12582917,
	25165843, 50331653, 100663319, 201326611, 
	402653189, 805306457, 1610612741
};

static void x_hmap_expand(x_hmap *ht, int elem_cnt)
{
	int new_slot_cnt, new_idx, new_load_limit;
	x_list *new_table;
	new_load_limit = ht->load_limit;
	new_slot_cnt = ht->slot_cnt;
	new_idx = ht->prime_idx;
	while(new_load_limit < elem_cnt && new_idx < x_arrlen(s_primes)) {
		new_slot_cnt = s_primes[++new_idx];
		new_load_limit = ht->load_factor * new_slot_cnt;
	}
	new_table = x_malloc(NULL, new_slot_cnt * sizeof(x_list));
	for(size_t i = 0; i < new_slot_cnt; ++i)
		x_list_init(new_table + i);
	for(size_t i = 0; i < ht->slot_cnt; ++i) {
		x_list_popeach(cur, ht->table) {
			size_t h = ht->hash(cur) % new_slot_cnt;
			x_list_add_back(new_table + h, cur);
		}
	}
	free(ht->table);
	ht->prime_idx = new_idx;
	ht->table = new_table;
	ht->slot_cnt = new_slot_cnt;
	ht->load_limit = new_load_limit;
}

void x_hmap_init(x_hmap *ht, float load_factor, x_hmap_hash_fn *hash_fn, x_hmap_equal_fn *equal_fn)
{
	int idx = 0;

	ht->prime_idx = 0;
	ht->load_limit = load_factor * s_primes[idx];
	ht->load_factor = load_factor;
	ht->elem_cnt = 0;
	ht->slot_cnt = s_primes[idx];

	ht->table = x_malloc(NULL, ht->slot_cnt * sizeof(x_link));
	for(size_t i = 0; i < ht->slot_cnt; ++i)
		x_list_init(ht->table + i);
}

static x_link *hmap_locate(x_hmap *ht, x_link *node, size_t *slot)
{
	*slot = ht->hash(node) % ht->slot_cnt;
	x_list_foreach(cur, ht->table + *slot)
		if (ht->equal(node, cur))
			return cur;
	return NULL;
}

x_link *x_hmap_find(x_hmap *ht, x_link *node)
{
	size_t slot;
	return hmap_locate(ht, node, &slot);
}

x_link *x_hmap_find_or_insert(x_hmap *ht, x_link *node)
{
	size_t slot;
	x_link *result = hmap_locate(ht, node, &slot);
	if (result)
		return result;

	if(ht->elem_cnt >= ht->load_limit)
		x_hmap_expand(ht, ht->elem_cnt + 1);

	x_list_add_back(ht->table + slot, node);
	ht->elem_cnt++;
	return 0;
}

x_link *x_hmap_replace_or_insert(x_hmap *ht, x_link *node)
{
	size_t slot;
	x_link *result = hmap_locate(ht, node, &slot);
	if (result) {
		x_list_replace(result, node);
		return result;
	}
	if(ht->elem_cnt >= ht->load_limit)
		x_hmap_expand(ht, ht->elem_cnt + 1);

	x_list_add_back(ht->table + slot, node);
	ht->elem_cnt++;
	return 0;
}

x_link *x_hmap_remove(x_hmap *ht, x_link *node)
{
	size_t slot;
	x_link *result = hmap_locate(ht, node, &slot);
	if (!result)
		return NULL;
	ht->elem_cnt--;
	return result;
}

void x_hmap_free(x_hmap * ht)
{
	free(ht->table);
}

