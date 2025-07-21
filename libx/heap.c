/*
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

#include "x/heap.h"
#include "x/assert.h"
#include "x/memory.h"
#include "x/string.h"
#include <stdlib.h>

#define PAGE_SIZE  4096
#define ENTRIES_PER_PAGE(h) (PAGE_SIZE / sizeof(x_ranode *))
#define LEFT_CHILD(i)   (((i)<<1)+1)
#define RIGHT_CHILD(i)  (((i)<<1)+2)
#define PARENT_ENTRY(i) (((i)-1)>>1)

void x_heap_init(x_heap* h, x_heap_ordered_fn *cmp)
{
	assert(h != NULL);
	assert(cmp != NULL);
	h->cmp = cmp;
	h->entry_cnt = 0;
	h->page_cnt = 1;
	h->min_page_cnt = h->page_cnt;
	h->table = x_malloc(NULL, h->page_cnt * PAGE_SIZE);
}

void x_heap_free(x_heap* h)
{
	if (!h)
		return;
	x_free(h->table);
}

inline static size_t x_heap_size(const x_heap* h)
{
	return h->entry_cnt;
}

x_ranode *x_heap_top(const x_heap* h)
{
	if (h->entry_cnt == 0)
		return 0;
	return h->table[0];
}

void x_heap_push(x_heap* h, x_ranode *p)
{
	assert(h->table != NULL);
	assert(p != NULL);

	int mx_entries = h->page_cnt * ENTRIES_PER_PAGE(h);
	if (h->entry_cnt + 1 > mx_entries) {
		int new_size = h->page_cnt * 2;
		x_ranode **new_table = x_realloc(h->table, new_size * PAGE_SIZE);
		h->table = new_table;
		h->page_cnt = new_size;
	}
	int current_index = h->entry_cnt, parent_index;
	x_ranode **current = h->table + current_index, **parent;
	while (current_index > 0) {
		parent_index = PARENT_ENTRY(current_index);
		parent = h->table + parent_index;
		if (!h->cmp(p, *parent))
			break;
		*current = *parent;
		(*parent)->index = current_index;
		current_index = parent_index;
		current = parent;
	}
	p->index = current_index;
	*current = p;
	h->entry_cnt++;
}

static void heap_shrink(x_heap* h)
{
	int used_pages = h->entry_cnt / ENTRIES_PER_PAGE(h) + ((h->entry_cnt % ENTRIES_PER_PAGE(h) > 0) ? 1 : 0);
	if (h->page_cnt / 2 > used_pages + 1 && h->page_cnt / 2 >= h->min_page_cnt) {
		int new_size = h->page_cnt / 2;
		h->table = x_realloc(h->table, new_size * PAGE_SIZE);
		h->page_cnt = new_size;
	}
}

x_ranode * x_heap_pop(x_heap* h)
{
	if (!h->entry_cnt)
		return NULL;
	h->entry_cnt--;

	x_ranode *top_value = h->table[0];
	int current_index = 0;
	x_ranode **current = h->table + current_index;
	int entries = h->entry_cnt;
	if (h->entry_cnt > 0) {
		*current = h->table[entries];
		(*current)->index = entries;
		x_ranode **left_child, **right_child;
		int left_child_index;
		while (left_child_index = LEFT_CHILD(current_index), left_child_index < entries) {
			left_child = h->table + left_child_index;
			if (left_child_index+ 1 < entries) {
				right_child = h->table + left_child_index + 1;
				if (!h->cmp(*right_child, *left_child)) {
					if (h->cmp(*left_child, *current)) {
						x_swap(current, left_child, x_ranode *);
						x_swap(*current, *left_child, x_ranode);
						current_index = left_child_index;
						current = left_child;
					}
					else
						break;
				}
				else {
					if (h->cmp(*right_child, *current)) {
						x_swap(current, right_child, x_ranode *);
						x_swap(*current, *right_child, x_ranode);
						current_index = left_child_index+1;
						current = right_child;
					}
					else
						break;
				}
			}
			else if (h->cmp(*left_child, *current)) {
				x_swap(current, left_child, x_ranode *);
				x_swap(*current, *left_child, x_ranode);
				current_index = left_child_index;
				current = left_child;
			}
			else
				break;
		}
	}
	heap_shrink(h);
	return top_value;
}

void x_heap_remove(x_heap *h, x_ranode *n)
{
	assert(h != NULL);
	assert(n != NULL);
	assert(n->index < h->entry_cnt);
	h->entry_cnt--;
    if (n->index == h->entry_cnt) {
		n->index = SIZE_MAX;
		return;
	}
	h->table[h->entry_cnt]->index = n->index;
	h->table[n->index] = h->table[h->entry_cnt];
    size_t current_idx = n->index;
    while (current_idx > 0) {
        size_t parent_idx = PARENT_ENTRY(current_idx);
		if (!h->cmp(h->table[current_idx], h->table[parent_idx]))
			break;
		x_swap(h->table + current_idx, h->table + parent_idx, x_ranode *);
		x_swap(h->table[current_idx], h->table[parent_idx], x_ranode);
		current_idx = parent_idx;
    }
    while (1) {
        size_t left = LEFT_CHILD(current_idx);
        size_t right = right = left + 1;
        size_t best = current_idx;

        if (left < h->entry_cnt && h->cmp(h->table[left], h->table[best]))
            best = left;
        if (right < h->entry_cnt && h->cmp(h->table[right], h->table[best]))
            best = right;

        if (best == current_idx)
            break;
		x_swap(h->table + current_idx, h->table + best, x_ranode *);
		x_swap(h->table[current_idx], h->table[best], x_ranode);
		current_idx = best;
    }
	heap_shrink(h);
}

