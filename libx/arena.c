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
#include "x/arena.h"
#include "x/memory.h"
#include <stddef.h>

#ifndef X_SEARCH_DEPTH
#define X_SEARCH_DEPTH 4
#endif

void x_arena_init1(x_arena *arena, size_t page_size)
{
	x_slist_init(&arena->pages);
	x_slist_init(&arena->large_pages);
	arena->page_size = page_size;
}

void x_arena_init(x_arena *arena)
{
	x_arena_init1(arena, X_ARENA_PAGE_SIZE);
}

static size_t round_powerof2(size_t n)
{
    n--;
    n |= n >> 1;
    n |= n >> 2;
    n |= n >> 4;
    n |= n >> 8;
    n |= n >> 16;
#if SIZE_MAX > 0xFFFFFFFF
    n |= n >> 32;
#endif
    n++;
    return n;
}

void *x_arena_alloc(x_arena *arena, size_t size)
{
	if (size > arena->page_size) {
		x_arena_page *large_page = x_malloc(NULL, sizeof *large_page + size);
		large_page->current = arena->page_size;
		x_slist_add_front(&arena->large_pages, &large_page->link);
		return large_page->region;
	}
	size_t align = x_min(round_powerof2(size), sizeof(long));
	x_slink *prev = &arena->pages.head;
	for (int i = 0; prev->next && i < X_SEARCH_DEPTH; i++) {
		x_arena_page *page = x_container_of(prev->next, x_arena_page, link);
		size_t aligned_cur = x_align(page->current, align ? align : 1);
		if (aligned_cur + size <= arena->page_size) {
			page->current = aligned_cur + size;
			return page->region + aligned_cur;
		}
		prev = prev->next;
	}
	x_arena_page *new_page = x_malloc(NULL, sizeof *new_page + arena->page_size);
	new_page->current = size;
	x_slist_add_front(&arena->pages, &new_page->link);
	return new_page->region;
}

void x_arena_free(x_arena *arena)
{
	x_slist_popeach(cur, &arena->pages)
		x_free(x_container_of(cur, x_arena_page, link));
	x_slist_popeach(cur, &arena->large_pages)
		x_free(x_container_of(cur, x_arena_page, link));
}

