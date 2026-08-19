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
#ifndef X_DEQBUF_H
#define X_DEQBUF_H

#include "x/types.h"
#include "x/ringbuf.h"
#include "x/memory.h"
#include "x/mutex.h"
#include "x/cond.h"

#define X_DEQBUF_SEGMENT_SIZE (1 << 9)
#define X_DEQBUF_FREE_LIMITS 8

#define X_DEQBUF_INIT3(var, _seg_size, _segs_max) { \
	.mset = X_MSET_INIT(var.mset), \
	.map = X_RINGBUF_INIT(NULL, 1), \
	.seg_size = _seg_size, \
	.segs_max = _segs_max, \
	.lock = X_MUTEX_INIT, \
	.cons_cond = X_COND_INIT, \
	.prod_cond = X_COND_INIT, \
}

#define X_DEQBUF_INIT(var) X_DEQBUF_INIT3(var,X_DEQBUF_SEGMENT_SIZE, SIZE_MAX / X_DEQBUF_SEGMENT_SIZE)

struct x_deqbuf_st
{
	x_mset mset;
	x_ringbuf map;
	size_t front, rear;
	size_t seg_size;
	size_t segs_max;
	x_mutex lock;
	x_cond cons_cond;
	x_cond prod_cond;
	void *free_list[X_DEQBUF_FREE_LIMITS];
	size_t free_cnt;
};

static inline void x_deqbuf_init3(x_deqbuf *deq, size_t seg_size, size_t segs_max)
{
	x_mset_init(&deq->mset);
	x_ringbuf_init(&deq->map, NULL, 1);
	deq->front = 0;
	deq->rear = 0;
	deq->free_cnt = 0;
	deq->seg_size = seg_size;
	deq->segs_max = segs_max;
	x_mutex_init(&deq->lock);
	x_cond_init(&deq->cons_cond);
	x_cond_init(&deq->prod_cond);
}

static inline void x_deqbuf_init(x_deqbuf *deq)
{
	x_deqbuf_init3(deq, X_DEQBUF_SEGMENT_SIZE, SIZE_MAX / X_DEQBUF_SEGMENT_SIZE);
}

static inline void x_deqbuf_free(x_deqbuf *deq)
{
	x_cond_destroy(&deq->cons_cond);
	x_cond_destroy(&deq->prod_cond);
	x_mutex_destroy(&deq->lock);
	x_mset_free(&deq->mset);
}

static inline size_t x_deqbuf_max_size(x_deqbuf *deq)
{
	return deq->segs_max * deq->seg_size;
}

size_t x_deqbuf_pop(x_deqbuf *deq, void *data, size_t size);
size_t x_deqbuf_push(x_deqbuf *deq, const void *data, size_t size);
size_t x_deqbuf_pop_back(x_deqbuf *deq, void *data, size_t size);
size_t x_deqbuf_push_front(x_deqbuf *deq, const void *data, size_t size);
size_t x_deqbuf_peek(x_deqbuf *deq, size_t start, void *buf, size_t size);
size_t x_deqbuf_peek_backward(x_deqbuf *deq, size_t start, void *buf, size_t size);
size_t x_deqbuf_modify(x_deqbuf *deq, size_t start, const void *data, size_t size);
size_t x_deqbuf_modify_backward(x_deqbuf *deq, size_t start, const void *data, size_t size);
void x_deqbuf_free(x_deqbuf *deq);
void x_deqbuf_clear(x_deqbuf *deq);
size_t x_deqbuf_size(x_deqbuf *deq);
bool x_deqbuf_is_empty(x_deqbuf *deq);
bool x_deqbuf_is_full(x_deqbuf *deq);

#endif

