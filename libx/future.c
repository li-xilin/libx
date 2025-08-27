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
#include <stdlib.h>
#include <stdio.h>

#define SEQ_ALIGN 0x10000

static int future_cmp(const x_btnode *n1, const x_btnode *n2)
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
	x_splay_init(&fup->fut_set, future_cmp);
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

static int alloc_freed_seq(x_fupool *fup)
{
	int freed_bit, start_bit = x_cyclic_inc(fup->last_id, SEQ_ALIGN);
	freed_bit = x_bitmap_find(&fup->id_map, 0, start_bit, SEQ_ALIGN - start_bit);
	if (freed_bit != -1)
		goto found;
	freed_bit = x_bitmap_find(&fup->id_map, 0, 0, fup->last_id);
	if (freed_bit == -1)
		return freed_bit;
found:
	x_bitmap_set(&fup->id_map, freed_bit, 1);
	fup->last_id = freed_bit;
	return freed_bit;
}

static void free_seq(x_fupool *fup, uint16_t seq)
{
	x_bitmap_set(&fup->id_map, seq, 1);
	x_cond_wake_all(&fup->seq_cond);
}

uint16_t x_fupool_alloc_seq(x_fupool *fup)
{
	int freed_seq;
	x_mutex_lock(&fup->lock);
	while ((freed_seq = alloc_freed_seq(fup)) == -1)
		x_cond_sleep(&fup->seq_cond, &fup->lock, -1);
	x_mutex_unlock(&fup->lock);
	return freed_seq;
}

void x_fupool_free_seq(x_fupool *fup, uint16_t seq)
{
	x_mutex_lock(&fup->lock);
	free_seq(fup, seq);
	x_mutex_unlock(&fup->lock);
}

uint16_t x_future_init(x_future *fut, x_fupool *fup, void *value)
{
	memset(&fut->value, 0 , sizeof fut->value);
	int freed_seq;
	x_mutex_lock(&fup->lock);
	while ((freed_seq = alloc_freed_seq(fup)) == -1)
		x_cond_sleep(&fup->seq_cond, &fup->lock, -1);
	fut->fupool = fup;
	fut->value.status = X_FUTURE_PENDING;
	fut->value.data = value;
	fut->seq = freed_seq;
	x_splay_find_or_insert(&fup->fut_set, &fut->node);
	x_mutex_unlock(&fup->lock);
	return freed_seq;
}

bool x_future_is_ready(x_future *fut)
{
	return fut->value.status == X_FUTURE_READY;
}

static int link_all_fut(x_future **fut_buf, size_t cnt, x_list *list)
{
	x_list_init(list);
	x_fupool *fup = NULL;
	int total = 0;
	for (int i = 0; i < cnt; i++) {
		if (fut_buf[i]->value.status != X_FUTURE_NULL) {
			if (!fup)
				fup = fut_buf[i]->fupool;
			else if (fup != fut_buf[i]->fupool) {
				errno = X_EINVAL;
				return -1;
			}
			x_list_add_back(list, &fut_buf[i]->link);
			total++;
		}
	}
	return total;
}

int x_future_wait_any(x_future **fut_buf, size_t cnt, int msec)
{
	int retval = -1;
	x_fupool *fup = NULL;
	x_list fut_list;
	int size = link_all_fut(fut_buf, cnt, &fut_list);
	if (size == -1)
		return -1;
	if (size == 0)
		return cnt;
	fup = x_container_of(x_list_first(&fut_list), x_future, link)->fupool;
	x_mutex_lock(&fup->lock);
	do {
		for (x_link *cur = fut_list.head.next; cur != &fut_list.head; ) {
			x_future *fut = x_container_of(cur, x_future, link);
			if (fut->value.status == X_FUTURE_NULL) {
				cur = cur->next;
				x_list_del(&fut->link);
				if (x_list_is_empty(&fut_list)) {
					retval = cnt;
					goto out;
				}
				continue;
			}
			if (fut->value.status == X_FUTURE_READY) {
				for (int i = 0; i < cnt; i++)
					if (fut_buf[i] == fut) {
						retval = i;
						goto out;
					}
				abort();
			}
			cur = cur->next;
		}
		if (x_cond_sleep(&fup->status_cond, &fup->lock, msec))
			goto out;
	} while(1);
out:
	x_mutex_unlock(&fup->lock);
	return retval;
}

int x_future_wait_all(x_future **fut_buf, size_t cnt, int msec)
{
	int retval = -1, have_ready = 0;
	x_fupool *fup = NULL;
	x_list fut_list;
	int size = link_all_fut(fut_buf, cnt, &fut_list);
	if (size == -1)
		return -1;
	if (size == 0)
		return cnt;
	fup = x_container_of(x_list_first(&fut_list), x_future, link)->fupool;
	x_mutex_lock(&fup->lock);
	x_link *cur;
	do {
		for (cur = fut_list.head.next; cur != &fut_list.head; ) {
			x_future *fut = x_container_of(cur, x_future, link);
			if (fut->value.status == X_FUTURE_NULL || fut->value.status == X_FUTURE_READY) {
				if (fut->value.status == X_FUTURE_READY)
					have_ready = 1;
				cur = cur->next;
				x_list_del(&fut->link);
				continue;
			}
			if (fut->value.status == X_FUTURE_PENDING || fut->value.status == X_FUTURE_BUSY) {
				if (x_cond_sleep(&fup->status_cond, &fup->lock, msec))
					goto out;
				break;
			}
			cur = cur->next;
		}
	} while (cur != fut_list.head.prev);

	retval = have_ready ? 0 : cnt;
out:
	x_mutex_unlock(&fup->lock);
	return retval;
}


int x_future_wait(x_future *fut, int msec)
{
	return x_future_wait_any(&fut, 1, msec);
}

void x_future_free(x_future *fut)
{
	if (!fut->fupool)
		return;
	x_mutex_lock(&fut->fupool->lock);
	while (fut->value.status == X_FUTURE_BUSY)
		x_cond_sleep(&fut->fupool->status_cond, &fut->fupool->lock, -1);
	free_seq(fut->fupool, fut->seq);
	x_splay_remove(&fut->fupool->fut_set, &fut->node);
	x_cond_wake_all(&fut->fupool->status_cond);
	fut->value.status = X_FUTURE_NULL;
	x_mutex_unlock(&fut->fupool->lock);
	fut->fupool = NULL;
}

void *x_promise_start(x_promise *prom, x_fupool *fup, uint16_t seq)
{
	void *data = NULL;
	x_mutex_lock(&fup->lock);
	x_future pattern = { .seq = seq };
	x_btnode *matched_node = x_splay_find(&fup->fut_set, &pattern.node);
	prom->fupool = fup;
	prom->seq = seq;
	if (!matched_node) {
		prom->value = NULL;
		goto out;
	}
	x_future *fut = x_container_of(matched_node, x_future, node);
	if (fut->value.status != X_FUTURE_PENDING) {
		prom->value = NULL;
		goto out;
	}
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
	if (!prom->value) {
		free_seq(prom->fupool, prom->seq);
		goto out;
	}
	x_mutex_lock(&prom->fupool->lock);
	prom->value->retcode = retcode;
	prom->value->status = X_FUTURE_READY;
	x_cond_wake_all(&prom->fupool->status_cond);
	x_mutex_unlock(&prom->fupool->lock);
out:
	prom->fupool = NULL;
	prom->seq = 0;
	prom->value = NULL;
}

