/*
  Copyright (c) 2025 Li Xilin <lixilin@gmx.com>

  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in
  all copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
  THE SOFTWARE.
*/
#include "x/future.h"
#include "x/cond.h"
#include "x/errno.h"
#include "x/list.h"
#include "x/macros.h"
#include "x/time.h"
#include <stdlib.h>
#include <stdio.h>

#define SEQ_ALIGN 0x10000

#define CHECK_FUTURE(fut) assert((fut)->value.status == X_FUTURE_PENDING \
		|| (fut)->value.status == X_FUTURE_READY \
		|| (fut)->value.status == X_FUTURE_BUSY)

int x_future_compare(const x_btnode *n1, const x_btnode *n2)
{
	x_future *fu1 = x_container_of(n1, x_future, node);
	x_future *fu2 = x_container_of(n2, x_future, node);
	return (int)fu1->seq - (int)fu2->seq;
}

void x_fupool_init(x_fupool *fup)
{
	x_mutex_init(&fup->lock);
	x_cond_init(&fup->status_cond);
	x_cond_init(&fup->seq_cond);
	x_bitmap_init(&fup->id_map, fup->bit_buf, sizeof fup->bit_buf);
	x_splay_init(&fup->fut_set, x_future_compare);
	memset(fup->bit_buf, 0 , sizeof fup->bit_buf);
	fup->last_id = 0xFFFF;
}

void x_fupool_free(x_fupool *fup)
{
	x_splay_foreach(cur, &fup->fut_set) {
		x_future *fut = x_container_of(cur, x_future, node);
		fut->fupool = NULL;
		fut->value.status = X_FUTURE_NULL;
	}
	x_mutex_destroy(&fup->lock);
	x_cond_destroy(&fup->status_cond);
	x_cond_destroy(&fup->seq_cond);
}

static int alloc_freed_seq(x_fupool *fup, uint16_t *seq)
{
	int freed_bit, start_bit = x_cyclic_inc(fup->last_id, SEQ_ALIGN);
	freed_bit = x_bitmap_find(&fup->id_map, 0, start_bit, SEQ_ALIGN - start_bit);
	if (freed_bit != -1)
		goto found;
	freed_bit = x_bitmap_find(&fup->id_map, 0, 0, fup->last_id);
	if (freed_bit == -1)
		return -1;
found:
	*seq = freed_bit;
	x_bitmap_set(&fup->id_map, freed_bit, 1);
	fup->last_id = freed_bit;
	return 0;
}

static void free_seq(x_fupool *fup, uint16_t seq)
{
	x_bitmap_set(&fup->id_map, seq, 0);
	x_cond_wake_all(&fup->seq_cond);
}

uint16_t x_fupool_alloc_seq(x_fupool *fup)
{
	uint16_t freed_seq;
	x_mutex_lock(&fup->lock);
	while (alloc_freed_seq(fup, &freed_seq) == -1)
		x_cond_sleep(&fup->seq_cond, &fup->lock, -1);
	x_mutex_unlock(&fup->lock);
	return freed_seq;
}

void x_fupool_free_seq(x_fupool *fup, uint16_t seq)
{
	x_mutex_lock(&fup->lock);
	free_seq(fup, (uint16_t)seq);
	x_mutex_unlock(&fup->lock);
}

uint16_t x_future_init(x_future *fut, x_fupool *fup, void *value)
{
	memset(&fut->value, 0 , sizeof fut->value);
	uint16_t freed_seq;
	x_mutex_lock(&fup->lock);
	while (alloc_freed_seq(fup, &freed_seq) == -1)
		x_cond_sleep(&fup->seq_cond, &fup->lock, -1);
	fut->fupool = fup;
	fut->value.status = X_FUTURE_PENDING;
	fut->value.data = value;
	fut->seq = freed_seq;
	fut->link.prev = fut->link.next = NULL;
	x_splay_find_or_insert(&fup->fut_set, &fut->node);
	x_mutex_unlock(&fup->lock);
	return freed_seq;
}

bool x_future_is_ready(x_future *fut)
{
	CHECK_FUTURE(fut);
	return fut->value.status == X_FUTURE_READY;
}

void x_fuset_add_many(x_fuset *set, x_future **fut_buf, size_t n)
{
	x_fupool *prev_fup = NULL;
	if (!x_list_is_empty(&set->list))
		prev_fup = x_container_of(x_list_last(&set->list), x_future, link)->fupool;
	for (int i = 0; i < n; i++) {
		if (!prev_fup)
			prev_fup = fut_buf[i]->fupool;
		else {
			assert(prev_fup == fut_buf[i]->fupool);
		}
		assert(!fut_buf[i]->link.prev);
		x_list_add_back(&set->list, &fut_buf[i]->link);
	}
}

void x_fuset_add(x_fuset *set, x_future *fut)
{
	x_fuset_add_many(set, &fut, 1);
}

 x_future *x_fuset_wait_any(x_fuset *set, int msec)
{
	x_future *any = NULL;
	x_fupool *fup = NULL;
	if (x_list_is_empty(&set->list))
		return NULL;
	fup = x_container_of(x_list_first(&set->list), x_future, link)->fupool;
	x_mutex_lock(&fup->lock);
	struct timeval tv1, tv2, diff;
	x_time_now(&tv1);
	while (1) {
		x_list_foreach(cur, &set->list) {
			x_future *fut = x_container_of(cur, x_future, link);
			CHECK_FUTURE(fut);
			if (fut->value.status == X_FUTURE_READY) {
				any = fut;
				goto out;
			}
		}
		if (x_cond_sleep(&fup->status_cond, &fup->lock, msec))
			goto out;
		x_time_now(&tv2);
		x_time_diff(tv2, tv1, &diff);
		msec -= x_time_to_msec(diff);
		tv1 = tv2;
	}
out:
	x_mutex_unlock(&fup->lock);
	return any;
}

int x_fuset_wait_all(x_fuset *set, int msec)
{
	int retval = -1;
	x_fupool *fup = NULL;
	if (x_list_is_empty(&set->list))
		return 0;
	fup = x_container_of(x_list_first(&set->list), x_future, link)->fupool;
	x_mutex_lock(&fup->lock);
	struct timeval tv1, tv2, diff;
	x_time_now(&tv1);
	while (1) {
		x_list_foreach(cur, &set->list) {
			x_future *fut = x_container_of(cur, x_future, link);
			CHECK_FUTURE(fut);
			if (fut->value.status != X_FUTURE_READY) {
				x_cond_sleep(&fup->status_cond, &fup->lock, msec);
				x_time_now(&tv2);
				x_time_diff(tv2, tv1, &diff);
				msec -= x_time_to_msec(diff);
				if (msec <= 0)
					goto out;
				tv1 = tv2;
				continue;
			}
		}
		break;
	}
	retval = 0;
out:
	x_mutex_unlock(&fup->lock);
	return retval;
}

void x_fuset_init(x_fuset *set)
{
	x_list_init(&set->list);
}

int x_future_wait(x_future *fut, int msec)
{
	x_fuset set;
	x_fuset_init(&set);
	x_fuset_add(&set, fut);
	return x_fuset_wait_all(&set, msec);
}

void x_future_free(x_future *fut)
{
	if (!fut->fupool)
		return;
	x_mutex_lock(&fut->fupool->lock);
	while (fut->value.status == X_FUTURE_BUSY) {
		x_cond_sleep(&fut->fupool->status_cond, &fut->fupool->lock, -1);
	}
	x_splay_remove(&fut->fupool->fut_set, &fut->node);
	if (fut->value.status == X_FUTURE_PENDING)
		free_seq(fut->fupool, fut->seq);
	fut->value.status = X_FUTURE_NULL;
	x_mutex_unlock(&fut->fupool->lock);
	if (fut->link.prev)
		x_list_del(&fut->link);
	fut->fupool = NULL;
}

void *x_promise_start(x_promise *prom, x_fupool *fup, uint16_t seq)
{
	void *data = NULL;
	x_mutex_lock(&fup->lock);
	x_future pat = { .seq = seq };
	x_btnode *matched_node = x_splay_find(&fup->fut_set, &pat.node);
	prom->fupool = fup;
	prom->seq = seq;
	if (!matched_node) {
		prom->value = NULL;
		goto out;
	}
	x_future *fut = x_container_of(matched_node, x_future, node);
	if (fut->value.status != X_FUTURE_PENDING)
		abort();
	prom->value = &fut->value;
	fut->value.status = X_FUTURE_BUSY;
	x_cond_wake_all(&prom->fupool->status_cond);
	data = fut->value.data;
out:
	x_mutex_unlock(&fup->lock);
	return data;
}

void x_promise_commit(x_promise *prom, int retcode)
{
	x_mutex_lock(&prom->fupool->lock);
	if (!prom->value)
		goto out;
	assert(prom->value->status == X_FUTURE_BUSY);
	prom->value->retcode = retcode;
	prom->value->status = X_FUTURE_READY;
	x_cond_wake_all(&prom->fupool->status_cond);
	free_seq(prom->fupool, prom->seq);
out:
	x_mutex_unlock(&prom->fupool->lock);
	prom->fupool = NULL;
	prom->seq = 0;
	prom->value = NULL;
}

