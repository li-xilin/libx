/*
 * Copyright (c) 2024-2026 Li Xilin <lixilin@gmx.com>
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
#ifndef X_RINGBUF_H
#define X_RINGBUF_H

#include "linkage.h"
#include "types.h"
#include "flowctl.h"
#include "assert.h"

X_EXTERN_C_BEGIN

#define X_RINGBUF_INIT(p, sz) { (char *)(p), (sz) }

struct x_ringbuf_st
{
	char *buf;
	size_t size, front, rear;
};

static inline void x_ringbuf_init(x_ringbuf *b, void *buf, size_t size)
{
	x_assert(size > 0);
	b->size = size;
	b->buf = buf;
	b->rear = b->front = 0;
}

static inline size_t x_ringbuf_capacity(const x_ringbuf *b)
{
	return b->size - 1;
}

static inline size_t x_ringbuf_used_size(const x_ringbuf *b)
{
	return (b->size + b->rear - b->front) % b->size;
}

static inline size_t x_ringbuf_free_size(const x_ringbuf *b)
{
	return x_ringbuf_capacity(b) - x_ringbuf_used_size(b);
}

static inline bool x_ringbuf_is_full(const x_ringbuf *b)
{
	return (b->rear + 1) % b->size == b->front;
}

static inline bool x_ringbuf_is_empty(const x_ringbuf *b)
{
	return b->rear == b->front;
}

static inline size_t __x_ringbuf_read_size(const x_ringbuf *b)
{
	return (b->rear >= b->front)
		? b->rear - b->front
		: b->size - b->front;
}

static inline const x_iovec x_ringbuf_acquire_read(const x_ringbuf *b)
{
	x_iovec slice;
	slice.base = b->buf + b->front;
	slice.len = __x_ringbuf_read_size(b);
	return slice;
}

static inline void x_ringbuf_commit_read(x_ringbuf *b, size_t size)
{
	x_assert(size <= __x_ringbuf_read_size(b));
	b->front = (b->front + size) % b->size;
}

static inline size_t __x_ringbuf_read_back_size(const x_ringbuf *b)
{
	return (b->rear >= b->front)
		? b->rear - b->front
		: (b->rear ? b->rear : b->size - b->front);
}

static inline const x_iovec x_ringbuf_acquire_read_back(const x_ringbuf *b)
{
	x_iovec slice;
	slice.base =  (b->rear >= b->front)
		? b->buf + b->front
		: (b->rear ? b->buf : b->buf + b->front);
	slice.len = __x_ringbuf_read_back_size(b);
	return slice;
}

static inline void x_ringbuf_commit_read_back(x_ringbuf *b, size_t size)
{
	x_assert(size <= __x_ringbuf_read_back_size(b));
	b->rear = (b->rear + b->size - size) % b->size;
}

static inline size_t __x_ringbuf_write_size(const x_ringbuf *b)
{
	return (b->rear >= b->front)
		? b->size - b->rear - !b->front
		: b->front - b->rear - 1;
}

static inline x_iovec x_ringbuf_acquire_write(x_ringbuf *b)
{
	x_iovec slice;
	slice.base = b->buf + b->rear;
	slice.len = __x_ringbuf_write_size(b);
	return slice;
}

static inline void x_ringbuf_commit_write(x_ringbuf *b, size_t size)
{
	x_assert(size <= __x_ringbuf_write_size(b));
	b->rear = (b->rear + size) % b->size;
}

static inline size_t __x_ringbuf_write_front_size(const x_ringbuf *b)
{
	return (b->rear >= b->front)
		? (b->front ? b->front : b->size - b->rear - 1 )
		: b->front - b->rear - 1;
}

static inline x_iovec x_ringbuf_acquire_write_front(x_ringbuf *b)
{
	x_iovec slice;
	slice.base =  (b->rear >= b->front)
		? (b->front ? b->buf : b->buf + b->rear + 1)
		: b->buf + b->rear + 1;
	slice.len = __x_ringbuf_write_front_size(b);
	return slice;
}

static inline void x_ringbuf_commit_write_front(x_ringbuf *b, size_t size)
{
	x_assert(size <= __x_ringbuf_write_front_size(b));
	b->rear = (b->front + b->size - size) % b->size;
}

static inline void x_ringbuf_clear(x_ringbuf *b)
{
	b->rear = b->front = 0;
}

size_t x_ringbuf_read(x_ringbuf *b, void *buf, size_t size);
size_t x_ringbuf_write(x_ringbuf *b, const void *data, size_t size);
size_t x_ringbuf_read_back(x_ringbuf *b, void *buf, size_t size);
size_t x_ringbuf_write_front(x_ringbuf *b, const void *data, size_t size);
size_t x_ringbuf_pour(x_ringbuf *src, x_ringbuf *dst, size_t size);
size_t x_ringbuf_pour_backward(x_ringbuf *src, x_ringbuf *dst, size_t size);
size_t x_ringbuf_copy(x_ringbuf *src, size_t start, size_t size, x_ringbuf *dst);
size_t x_ringbuf_copy_backward(x_ringbuf *src, size_t start, size_t size, x_ringbuf *dst);
size_t x_ringbuf_peek(x_ringbuf *b, void *buf, size_t start, size_t size);
size_t x_ringbuf_peek_backward(x_ringbuf *b, void *buf, size_t start, size_t size);
void x_ringbuf_resize(x_ringbuf *ringbuf, size_t size);
void x_ringbuf_compact(x_ringbuf *b);
void *x_ringbuf_linearize(x_ringbuf *b);

X_EXTERN_C_END

#endif

