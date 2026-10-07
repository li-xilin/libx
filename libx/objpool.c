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
#include "x/objpool.h"
#include "x/memory.h"
#include "x/bitmap.h"
#include "x/assert.h"
#include "x/thread.h"
#include "x/sys.h"
#include "x/once.h"
#include <stdio.h>

#define SLOTS_ALL_ZERO ((uint64_t) 0)
#define SLOTS_FIRST ((uint64_t) 1)
#define FREE_SLOT_COUNT(x) X_BITMAP64_ONE_COUNT(x)
#define FIRST_FREE_SLOT(x) X_BITMAP64_FIRST_ONE(x)

static size_t s_pagesize = 0;
static x_once s_pagesize_once = X_ONCE_INIT;

static void init_pagesize(void)
{
	s_pagesize = x_page_size();
	x_assert(x_powerof2(s_pagesize));
}

static inline bool is_uint16(uintptr_t x)
{
	return ((x) > 0 && (x) < 0x10000);
}

static inline void slab_refcnt_add(x_slab *slab, int n)
{
	if (is_uint16(slab->refcnt_or_head))
		slab->refcnt_or_head += n;
	else
		((x_slab *)(slab->refcnt_or_head))->refcnt_or_head += n; 
}

static inline x_slab *slab_head(x_slab *slab)
{
	return (is_uint16(slab->refcnt_or_head) ? (slab) : (x_slab *)slab->refcnt_or_head);
}

static inline int slab_refcnt(const x_slab *slab)
{
	return (is_uint16((slab)->refcnt_or_head)
			? (slab)->refcnt_or_head
			: ((x_slab *)((slab)->refcnt_or_head))->refcnt_or_head);
}

static inline bool one_used_slot(uint64_t slots, uint64_t empty_slotmask)
{
	uint64_t a = ~(slots) & (empty_slotmask);
	uint64_t b = a - 1;
	return (a & b) == SLOTS_ALL_ZERO;
}

static inline x_slab *slab_locate(x_objpool *pool, void *ptr)
{
	return (x_slab *) ((uintptr_t)ptr & ~(pool->slab_size - 1));
}

static inline size_t round_up_pow2(size_t x)
{
	if (x == 0 || x_powerof2(x))
		return x;
	x--;
	x |= x >> 1;
	x |= x >> 2;
	x |= x >> 4;
	x |= x >> 8;
	x |= x >> 16;
#if SIZE_MAX > UINT32_MAX
	x |= x >> 32;
#endif
	return x + 1;
}

static inline size_t slab_item_count(const x_objpool *pool)
{
	const size_t data_offset = offsetof(x_slab, data);
	return (pool->slab_size - data_offset) / pool->item_size;
}

static void objpool_reset(x_objpool *pool)
{
	x_assert(pool->item_size);
	x_once_init(&s_pagesize_once, init_pagesize);
	const size_t data_offset = offsetof(x_slab, data);
	const size_t least_slab_size = data_offset + 64 * pool->item_size;
	pool->slab_size = round_up_pow2(least_slab_size);
	size_t item_count = 64;
	if (pool->slab_size - least_slab_size != 0) {
		const size_t shrinked_slab_size = pool->slab_size >> 1;
		if (data_offset < shrinked_slab_size && shrinked_slab_size - data_offset >= 2 * pool->item_size) {
			pool->slab_size = shrinked_slab_size;
			item_count = (shrinked_slab_size - data_offset) / pool->item_size;
		}
	}
	pool->block_size = pool->slab_size > s_pagesize ?  pool->slab_size : s_pagesize;
	pool->empty_slotmask = ~SLOTS_ALL_ZERO >> (64 - item_count);
	x_list_init(&pool->partial);
	x_list_init(&pool->empty);
	x_list_init(&pool->full);
}

void x_objpool_init(x_objpool *pool, size_t item_size)
{
	x_assert(pool != NULL);
	x_assert(item_size >= 1);
	pool->item_size = item_size;
	objpool_reset(pool);
}

void x_objpool_free(x_objpool *pool)
{
	if (!pool || !pool->block_size)
		return;
	const x_list *lists[3] = { &pool->partial, &pool->empty, &pool->full };
	for (int i = 0; i < x_arrlen(lists); i++) {
		while (X_LIKELY(!x_list_is_empty(lists[i]))) {
			x_slab *slab = x_container_of(x_list_first(lists[i]), x_slab, link);
			x_slab *head_slab = slab_head(slab);
			const char *slab_end = (char *)head_slab + pool->block_size;
			for (slab = head_slab; (const char *)slab < slab_end; slab = (x_slab *)((char *)slab + pool->slab_size))
				x_list_del(&slab->link);
			x_aligned_free(head_slab);
		}
	}
}

void *x_objpool_alloc(x_objpool *pool)
{
	x_assert(pool != NULL);
	if (pool->block_size == 0) {
		x_assert(pool->item_size > 0);
		objpool_reset(pool);
	}
	if (X_LIKELY(!x_list_is_empty(&pool->partial))) {
		x_slab *slab = x_container_of(x_list_first(&pool->partial), x_slab, link);
		register const size_t slot = FIRST_FREE_SLOT(slab->slots);
		slab->slots ^= SLOTS_FIRST << slot;
		void *ptr = slab->data + slot * pool->item_size;
		if (X_UNLIKELY(slab->slots == SLOTS_ALL_ZERO)) {
			x_list_del(&slab->link);
			x_list_add_back(&pool->full, &slab->link);
		}
		return ptr;
	}
	else if (X_LIKELY(!x_list_is_empty(&pool->empty))) {
		x_slab *slab = x_container_of(x_list_first(&pool->empty), x_slab, link);
		x_list_del(&slab->link);
		x_list_add_back(&pool->partial, &slab->link);
		slab_refcnt_add(slab, 1);
		slab->slots = pool->empty_slotmask ^ SLOTS_FIRST;
		return slab->data;
	}
	else {
		x_slab *head_slab = x_aligned_alloc_retry(pool->slab_size, pool->block_size);
		head_slab->refcnt_or_head = 1;
		head_slab->slots = pool->empty_slotmask;
		head_slab->slots ^= SLOTS_FIRST;
		x_list_add_front(&pool->partial, &head_slab->link);
		x_slab *slab = (x_slab *)((char *)head_slab + pool->slab_size);
		const char *slab_end = (char *)head_slab + pool->block_size;
		while (X_LIKELY((const char *)slab < slab_end)) {
			slab->refcnt_or_head = (uintptr_t)head_slab;
			slab->slots = pool->empty_slotmask;
			x_list_add_front(&pool->empty, &slab->link);
			slab = (x_slab *)((char *)slab + pool->slab_size);
		}
		return head_slab->data;
	}
}

void x_objpool_dealloc(x_objpool *pool, void *ptr)
{
	x_assert(pool != NULL);
	x_assert(ptr != NULL);
	x_slab *slab = slab_locate(pool, ptr);
	const int slot = ((char *) ptr - (char *) slab - offsetof(x_slab, data)) / pool->item_size;

	if (X_UNLIKELY(slab->slots == SLOTS_ALL_ZERO)) {
		slab->slots = SLOTS_FIRST << slot;
		x_list_del(&slab->link);
		x_list_add_front(&pool->partial, &slab->link);
	}
	else if (X_UNLIKELY(one_used_slot(slab->slots, pool->empty_slotmask))) {
		if (X_UNLIKELY(slab_refcnt(slab) == 1)) {
			x_slab *head_slab = slab_head(slab);
			slab = head_slab;
			const char *slab_end = (char *)head_slab + pool->block_size;
			while ((const char *)slab < slab_end) {
				x_list_del(&slab->link);
				slab = (x_slab *)((char *)slab + pool->slab_size);
			}   
			x_aligned_free(head_slab);
		}
		else {
			slab->slots = pool->empty_slotmask;
			x_list_del(&slab->link);
			x_list_add_front(&pool->empty, &slab->link);
			slab_refcnt_add(slab, -1);
		}
	}
	else {
		slab->slots |= SLOTS_FIRST << slot;
	}
}

static void slab_stats(FILE *out, const x_objpool *pool)
{
	x_assert(out != NULL);
	x_assert(pool != NULL);
	long long unsigned total_nr_slabs = 0, total_used_slots = 0,
		 total_free_slots = 0;
	float occupancy;
	const char *labels[] = {"Partial", "Empty", "Full"};
	size_t item_count = slab_item_count(pool);

	fprintf(out, "%8s %17s %17s %17s %17s\n", "",
			"Slabs", "Used", "Free", "Occupancy");
	const x_list *lists[3] = { &pool->partial, &pool->empty, &pool->full };
	for (int i = 0; i < x_arrlen(lists); i++) {
		long long unsigned nr_slabs = 0, used_slots = 0, free_slots = 0;
		x_list_foreach(pos, lists[i]) {
			x_slab *slab = x_container_of(pos, x_slab, link);
			nr_slabs++;
			used_slots += item_count - FREE_SLOT_COUNT(slab->slots);
			free_slots += FREE_SLOT_COUNT(slab->slots);
		}
		occupancy = used_slots + free_slots ?
			100 * (float) used_slots / (used_slots + free_slots) : 0.0;
		fprintf(out, "%8s %17llu %17llu %17llu %16.2f%%\n",
				labels[i], nr_slabs, used_slots, free_slots, occupancy);
		total_nr_slabs += nr_slabs;
		total_used_slots += used_slots;
		total_free_slots += free_slots;
	}
	occupancy = total_used_slots + total_free_slots ?
		100 * (float) total_used_slots / (total_used_slots + total_free_slots) : 0.0;
	fprintf(out, "%8s %17llu %17llu %17llu %16.2f%%\n", "Total",
			total_nr_slabs, total_used_slots, total_free_slots, occupancy);
}

static void slab_props(FILE *out, const x_objpool *pool)
{
	x_assert(out != NULL);
	x_assert(pool != NULL);
	size_t item_count = slab_item_count(pool);
	fprintf(out,
			"%18s: %8zu\n"
			"%18s: %8zu\n"
			"%18s: %8zu = (%zu offset) + (%zu item_count) * (%zu item_size)\n"
			"%18s: %8zu = (%zu slab_size) - (%zu total)\n"
			"%18s: %8zu = %zu * (%zu pagesize)\n"
			"%18s: %8zu = (%zu alloc) / (%zu slab_size)\n",
		"pagesize",
			s_pagesize,
		"slab_size",
			pool->slab_size, 
		"total",
			offsetof(x_slab, data) + item_count * pool->item_size,
			offsetof(x_slab, data), item_count, pool->item_size,
		"waste per slab",
			pool->slab_size - offsetof(x_slab, data) - item_count * pool->item_size, pool->slab_size,
			offsetof(x_slab, data) + item_count * pool->item_size,
		"size per alloc",
			pool->block_size, pool->block_size / s_pagesize,
			s_pagesize,
		"slabs per alloc",
			pool->block_size / pool->slab_size, pool->block_size,
			pool->slab_size);
}

void x_objpool_show(const x_objpool *pool, void *out)
{
	x_assert(out != NULL);
	x_assert(pool != NULL);
	slab_props(out, pool);
	fputc('\n', out);
	slab_stats(out, pool);
	fputc('\n', out);
	const char *labels[] = {"part", "empt", "full"};
	const x_list *lists[3] = { &pool->partial, &pool->empty, &pool->full };
	size_t item_count = slab_item_count(pool);
	for (int i = 0; i < x_arrlen(lists); i++) {
		fprintf(out,
				"%6s " "|%2d%13s|%2d%13s|%2d%13s|%2d%13s" "\n",
				labels[i], 64, "", 48, "", 32, "", 16, "");
		unsigned long long total = 0, row = 1;
		x_list_foreach(pos, lists[i]) {
			x_slab *slab = x_container_of(pos, x_slab, link);
			const unsigned used = item_count - FREE_SLOT_COUNT(slab->slots);
			fprintf(out, "%6llu ", row);
			for (int k = 63; k >= 0; --k) {
				fprintf(out, slab->slots & (SLOTS_FIRST << k) ? "1" :
						((size_t) k >= item_count ? "0" : "0"));
			}
			fprintf(out, " %8u" "\n", used);
			total += used;
		}
		fprintf(out, "%6s " "^%15s^%15s^%15s^%15s" " %8llu" "\n\n",
				"", "", "", "", "", total);
	}
}

bool x_objpool_sanitize(const x_objpool *pool)
{
	size_t item_count = slab_item_count(pool);
	if (!x_powerof2(s_pagesize)) {
		x_sanitize_stub();
		return false;
	}
	if (!x_powerof2(pool->slab_size)) {
		x_sanitize_stub();
		return false;
	}
	if (!x_powerof2(pool->block_size)) {
		x_sanitize_stub();
		return false;
	}
	if (pool->item_size < 1) {
		x_sanitize_stub();
		return false;
	}
	if (pool->block_size < s_pagesize) {
		x_sanitize_stub();
		return false;
	}
	if (pool->block_size < pool->slab_size) {
		x_sanitize_stub();
		return false;
	}
	if (pool->empty_slotmask != ~SLOTS_ALL_ZERO >> (64 - item_count)) {
		x_sanitize_stub();
		return false;
	}
	const x_list *lists[3] = { &pool->full, &pool->empty, &pool->partial };
	for (int i = 0; i < x_arrlen(lists); i++) {
		x_list_foreach(pos, lists[i]) {
			x_slab *slab = x_container_of(pos, x_slab, link);
			x_link_sanitize(&slab->link);
			if (i == 0) {
				if (slab->slots != SLOTS_ALL_ZERO) {
					x_sanitize_stub();
					return false;
				}
			}
			else if (i == 1) {
				if (slab->slots != pool->empty_slotmask) {
					x_sanitize_stub();
					return false;
				}
			}
			else if (i == 2) {
				if ((slab->slots & ~pool->empty_slotmask) != SLOTS_ALL_ZERO) {
					x_sanitize_stub();
					return false;
				}
				if (FREE_SLOT_COUNT(slab->slots) < 1) {
					x_sanitize_stub();
					return false;
				}
				if (FREE_SLOT_COUNT(slab->slots) > item_count) {
					x_sanitize_stub();
					return false;
				}
			}
			if ((uintptr_t) slab % pool->slab_size != 0) {
				x_sanitize_stub();
				return false;
			}
		}
	}
	return true;
}

