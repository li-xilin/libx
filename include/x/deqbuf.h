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

#include "linkage.h"
#include "types.h"
#include "ringbuf.h"
#include "memory.h"
#include "mutex.h"
#include "cond.h"

X_EXTERN_C_BEGIN

#define X_DEQBUF_SEGMENT_SIZE (1 << 9)
#define X_DEQBUF_FREE_LIMITS 8
#define X_DEQBUF_INIT_1(var, _seg_size, _segs_max) { \
	X_MSET_INIT(var.mset), \
	X_RINGBUF_INIT(NULL, 1), \
	0, 0, \
	_seg_size, \
	_segs_max, \
	X_MUTEX_INIT, \
	X_COND_INIT, \
	X_COND_INIT, \
}
#define X_DEQBUF_INIT(var) X_DEQBUF_INIT_1(var,X_DEQBUF_SEGMENT_SIZE, SIZE_MAX / X_DEQBUF_SEGMENT_SIZE)

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

void x_deqbuf_init1(x_deqbuf *deq, size_t seg_size, size_t segs_max);
void x_deqbuf_init(x_deqbuf *deq);
void x_deqbuf_free(x_deqbuf *deq);
size_t x_deqbuf_max_size(x_deqbuf *deq);
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

X_EXTERN_C_END

#endif

