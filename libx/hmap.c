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

#include "x/hmap.h"
#include "x/list.h"
#include "x/memory.h"
#include "x/debug.h"
#include <stdlib.h>

static const size_t s_prime_table[] =
{
	0, 53, 97, 193, 389, 769, 1543, 3079, 6151, 12289, 24593, 49157, 98317,
	196613, 393241, 786433, 1572869, 3145739, 6291469, 12582917, 25165843,
	50331653, 100663319, 201326611, 402653189, 805306457, 1610612741,
#if X_WORDSIZE == 64
	3221225473, 6442450939, 12884901893, 25769803751, 51539607551, 103079215111,
	206158430209, 412316860441, 824633720831, 1649267441651, 3298534883309,
	6597069766657, 13194139533299, 26388279066623, 52776558133303, 105553116266489,
	211106232532969, 422212465066001, 844424930131963, 1688849860263953,
	3377699720527861, 6755399441055731, 13510798882111483, 27021597764222939,
	54043195528445957, 108086391056891903, 216172782113783843, 432345564227567621,
	864691128455135207, 1729382256910270481, 3458764513820540933, 6917529027641081903,
	9223372036854775783,
#endif
};

static void hmap_expand(x_hmap *ht, int elem_cnt)
{
	int new_slot_cnt, new_idx, new_load_limit;
	x_slist *new_table;
	if (ht->prime_index == x_arrlen(s_prime_table) - 1) {
		x_assert_printf(elem_cnt <= s_prime_table[x_arrlen(s_prime_table) - 1], "elements exceed the max capacity limit");
		abort();
		return;
	}
	new_idx = ht->prime_index;
	do {
		new_idx++;
		new_slot_cnt = s_prime_table[new_idx];
		new_load_limit = ht->load_factor * new_slot_cnt;
	} while(new_load_limit < elem_cnt && new_idx < x_arrlen(s_prime_table));
	if (new_idx == x_arrlen(s_prime_table))
		abort();
	new_table = x_malloc(NULL, new_slot_cnt * sizeof(x_list));
	for(size_t i = 0; i < new_slot_cnt; ++i)
		x_slist_init(new_table + i);
	for(size_t i = 0; i < ht->slot_cnt; ++i) {
		x_slist_popeach(cur, ht->table + i) {
			x_hnode *node = x_container_of(cur, x_hnode, link);
			size_t h = node->hash % new_slot_cnt;
			x_slist_add_front(new_table + h, &node->link);
		}
	}
	x_free(ht->table);
	ht->prime_index = new_idx;
	ht->table = new_table;
	ht->slot_cnt = new_slot_cnt;
}

static inline size_t node_slot(const x_hmap *ht, const x_hnode *node)
{
	return node->hash % ht->slot_cnt;
}

static x_slink *hmap_locate_prev(const x_hmap *ht, const x_hnode *node, size_t slot)
{
	x_slist_foreach_prev(prev, ht->table + slot) {
		x_hnode *cur_node = x_container_of(prev->next, x_hnode, link);
		if (ht->equal(node, cur_node))
			return prev;
	}
	return NULL;
}

void x_hmap_init1(x_hmap *ht, float load_factor, x_hmap_equal_fn *equal_fn)
{
	ht->prime_index = 1;
	ht->load_factor = load_factor;
	ht->elem_cnt = 0;
	ht->slot_cnt = s_prime_table[ht->prime_index];
	ht->equal = equal_fn;
	ht->table = x_malloc(NULL, ht->slot_cnt * sizeof(x_hnode));
	for(size_t i = 0; i < ht->slot_cnt; ++i)
		x_slist_init(ht->table + i);
}

void x_hmap_init(x_hmap *ht, x_hmap_equal_fn *equal_fn)
{
	x_hmap_init1(ht, 0.5, equal_fn);
}

x_hnode *x_hmap_find(const x_hmap *ht, const x_hnode *node)
{
	if (!ht->slot_cnt)
		return NULL;
	size_t slot = node_slot(ht, node);
	x_slink *prev = hmap_locate_prev(ht, node, slot);
	if (!prev)
		return NULL;
	return x_container_of(prev->next, x_hnode, link);
}

x_hnode *x_hmap_find_or_insert(x_hmap *ht, x_hnode *node)
{
	if (!ht->slot_cnt)
		hmap_expand(ht, 1);
	size_t slot = node_slot(ht, node);
	x_slink *prev = hmap_locate_prev(ht, node, slot);
	if (prev)
		return x_container_of(prev->next, x_hnode, link);
	if (ht->elem_cnt >= ht->load_factor * s_prime_table[ht->prime_index]) {
		hmap_expand(ht, ht->elem_cnt + 1);
		slot = node_slot(ht, node);
	}
	x_slist_add_front(ht->table + slot, &node->link);
	ht->elem_cnt++;
	return NULL;
}

x_hnode *x_hmap_replace_or_insert(x_hmap *ht, x_hnode *node)
{
	if (!ht->slot_cnt)
		hmap_expand(ht, 1);
	size_t slot = node_slot(ht, node);
	x_slink *prev = hmap_locate_prev(ht, node, slot);
	if (prev) {
		x_slink *del = x_slink_replace_after(prev, &node->link);
		return x_container_of(del, x_hnode, link);
	}
	if (ht->elem_cnt >= ht->load_factor * s_prime_table[ht->prime_index]) {
		hmap_expand(ht, ht->elem_cnt + 1);
		slot = node_slot(ht, node);
	}
	x_slist_add_front(ht->table + slot, &node->link);
	ht->elem_cnt++;
	return 0;
}

x_hnode *x_hmap_find_and_remove(x_hmap *ht, const x_hnode *node)
{
	size_t slot = node_slot(ht, node);
	x_slink *prev = hmap_locate_prev(ht, node, slot);
	if (!prev)
		return NULL;
	x_slink *del = x_slink_del_after(prev);
	ht->elem_cnt--;
	return x_container_of(del, x_hnode, link);
}

void x_hmap_remove(x_hmap *ht, const x_hnode *node)
{
	size_t slot = node_slot(ht, node);
	x_slist_foreach_prev(prev, &ht->table[slot]) {
		if (prev->next == &node->link) {
			x_slink_del_after(prev);
			ht->elem_cnt--;
			return;
		}
	}
}

void x_hmap_free(x_hmap * ht)
{
	x_free(ht->table);
}

void x_hmap_clear(x_hmap *ht)
{
	x_free(ht->table);
	x_hmap_init1(ht, ht->load_factor, ht->equal);
}

bool x_hmap_sanitize(x_hmap *ht)
{
	int cnt = 0;
	for (int i = 0; i < ht->slot_cnt; i++) {
		x_slist_foreach(cur, ht->table + i) {
			cnt++;
			if (node_slot(ht, x_container_of(cur, x_hnode, link)) != i) {
				x_sanitize_stub();
				return false;
			}
		}
	}
	if (cnt != ht->elem_cnt) {
		x_sanitize_stub();
		return false;
	}
	return true;
}

