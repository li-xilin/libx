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
#define ENTRY_AT(h, index) (h->table + (index) * h->entry_size)
#define ENTRIES_PER_PAGE(h) (PAGE_SIZE / h->entry_size)
#define LEFT_CHILD(i)   (((i)<<1)+1)
#define RIGHT_CHILD(i)  (((i)<<1)+2)
#define PARENT_ENTRY(i) (((i)-1)>>1)

void x_heap_init(x_heap* h, size_t entry_size, size_t min_pages, x_heap_cmp_fn *cmp, void *ctx)
{
	x_assert_not_null(h);
	x_assert_not_null(cmp);
	assert(min_pages > 0);

	h->cmp = cmp;
	h->entry_cnt = 0;
	h->page_cnt = min_pages;
	h->entry_size = entry_size;
	h->min_page_cnt = h->page_cnt;
	h->ctx = ctx;
	h->table = (uint8_t *)x_malloc(NULL, h->page_cnt * PAGE_SIZE);
}

void x_heap_free(x_heap* h)
{
	if (!h)
		return;
	x_free(h->table);
	h->entry_cnt = 0;
	h->page_cnt = 0;
	h->table = NULL;
}

inline static size_t x_heap_size(const x_heap* h)
{
	return h->entry_cnt;
}

const void *x_heap_top(const x_heap* h)
{
	if (h->entry_cnt == 0)
		return 0;
	return (void *)ENTRY_AT(h, 0);
}

void x_heap_push(x_heap* h, const void *key)
{
	assert(h->table);

	int mx_entries = h->page_cnt * ENTRIES_PER_PAGE(h);
	if (h->entry_cnt + 1 > mx_entries) {
		int new_size = h->page_cnt * 2;
		uint8_t *new_table = (uint8_t *)x_realloc(h->table, new_size * PAGE_SIZE);
		h->table = new_table;
		h->page_cnt = new_size;
	}

	int current_index = h->entry_cnt;
	uint8_t *current = ENTRY_AT(h, current_index);

	int parent_index;
	uint8_t *parent;

	while (current_index > 0) {
		parent_index = PARENT_ENTRY(current_index);
		parent = ENTRY_AT(h, parent_index);
		if (h->cmp(key, parent, h->ctx)) {
			memcpy(current, parent, h->entry_size);
			current_index = parent_index;
			current = parent;

		} else
			break;
	}
	memcpy(current, key, h->entry_size);
	h->entry_cnt++;
}

void x_heap_pop(x_heap* h)
{
	assert(h->entry_cnt);

	int current_index = 0;
	uint8_t *current = ENTRY_AT(h, current_index);
	h->entry_cnt--;
	int entries = h->entry_cnt;
	if (h->entry_cnt > 0) {
		uint8_t *last = ENTRY_AT(h, entries);
		memcpy(current, last, h->entry_size);

		uint8_t *left_child, *right_child;
		int left_child_index;
		while (left_child_index = LEFT_CHILD(current_index), left_child_index < entries) {
			left_child = ENTRY_AT(h, left_child_index);

			if (left_child_index+ 1 < entries) {
				right_child = ENTRY_AT(h, left_child_index+1);
				if (!h->cmp(right_child, left_child, h->ctx)) {
					if (h->cmp(left_child, current, h->ctx)) {
						x_memswp(current, left_child, h->entry_size);
						current_index = left_child_index;
						current = left_child;
					}
					else
						break;
				}
				else {
					if (h->cmp(right_child, current, h->ctx)) {
						x_memswp(current, right_child, h->entry_size);
						current_index = left_child_index+1;
						current = right_child;
					}
					else
						break;
				}
			}
			else if (h->cmp(left_child, current, h->ctx)) {
				x_memswp(current, left_child, h->entry_size);
				current_index = left_child_index;
				current = left_child;

			}
		       	else
				break;
		}
	}

	int used_pages = entries / ENTRIES_PER_PAGE(h) + ((entries % ENTRIES_PER_PAGE(h) > 0) ? 1 : 0);
	if (h->page_cnt / 2 > used_pages + 1 && h->page_cnt / 2 >= h->min_page_cnt) {
		int new_size = h->page_cnt / 2;
		uint8_t *new_table = (uint8_t *)realloc(h->table, new_size * PAGE_SIZE);
		if (new_table)
			h->table = new_table;

		h->table = new_table;
		h->page_cnt = new_size;
	}
}

