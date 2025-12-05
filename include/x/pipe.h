/*
 * Copyright (c) 2024 Li Xilin <lixilin@gmx.com>
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

#ifndef X_PIPE_H
#define X_PIPE_H

#include "types.h"
#include <string.h>
#include <assert.h>

struct x_pipe_st
{
	size_t size, front, rear;
	uint8_t *buf;
};

typedef void x_pipe_drain_f(void *data, size_t size, void *arg);

inline static void x_pipe_init(x_pipe *b, void *buf, size_t size)
{
	assert(size > 1);
	b->size = size;
	b->buf = (uint8_t *)buf;
	b->rear = b->front = 0;
}

inline static size_t x_pipe_data_size(x_pipe *b)
{
	return (b->size + b->rear - b->front) % b->size;
}

inline static bool x_pipe_is_full(x_pipe *b)
{
	return (b->rear + 1) % b->size == b->front;
}

inline static bool x_pipe_is_empty(x_pipe *b)
{
	return b->rear == b->front;
}

inline static size_t x_pipe_max_size(x_pipe *b)
{
	return b->size - 1;
}

inline static size_t x_pipe_buffer_size(x_pipe *b)
{
	return x_pipe_max_size(b) - x_pipe_data_size(b);
}

inline static void *x_pipe_zread(const x_pipe *b)
{
	return b->buf + b->front;
}

inline static size_t x_pipe_zread_size(const x_pipe *b)
{
	return (b->rear >= b->front)
		? b->rear - b->front
		: b->size - b->front;
}

inline static void x_pipe_zread_commit(x_pipe *b, size_t size)
{
	assert(size <= x_pipe_zread_size(b));
	b->front = (b->front + size) % b->size;
}

inline static void *x_pipe_zwrite(const x_pipe *b)
{
		return b->buf + b->rear;
}

inline static size_t x_pipe_zwrite_size(const x_pipe *b)
{
	return (b->rear >= b->front)
		? b->size - b->rear - !b->front
		: b->front - b->rear - 1;
}

inline static void x_pipe_zwrite_commit(x_pipe *b, size_t size)
{
	assert(size <= x_pipe_zwrite_size(b));
	b->rear = (b->rear + size) % b->size;
}

inline static void x_pipe_clear(x_pipe *b)
{
	b->rear = b->front = 0;
}

size_t x_pipe_write(x_pipe *b, void *p, size_t size);

size_t x_pipe_pour(x_pipe *src, x_pipe *dst, size_t size);

size_t x_pipe_peek(x_pipe *b, void *buf, size_t start, size_t size);

size_t x_pipe_read(x_pipe *b, void *buf, size_t size);

void *x_pipe_change_buffer(x_pipe *b, void *buf, size_t size);

size_t x_pipe_drain(x_pipe *b, size_t size, x_pipe_drain_f *cb, void *arg);

void *x_pipe_pullup(x_pipe *b);

inline static void x_pipe_refront(x_pipe *b)
{
	if (b->front <= b->rear)
		memmove(b->buf, b->buf + b->front, x_pipe_data_size(b));
	else
		x_pipe_pullup(b);

	b->rear = x_pipe_data_size(b);
	b->front = 0;
}

#endif
