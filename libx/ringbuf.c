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

#include "x/ringbuf.h"
#include "x/string.h"
#include <errno.h>

size_t x_ringbuf_write(x_ringbuf *rb, const void *data, size_t size)
{
	const char *p = data;
	size_t total_size = x_min(x_ringbuf_free_size(rb), size);
	size_t rear_buf_size = x_min(rb->size - rb->rear, total_size);
	memcpy(rb->buf + rb->rear, p, rear_buf_size);
	memcpy(rb->buf, p + rear_buf_size, total_size - rear_buf_size);
	rb->rear = (rb->rear + total_size) % rb->size;
	return total_size;
}

size_t x_ringbuf_write_front(x_ringbuf *rb, const void *data, size_t size)
{
	const char *p = data;
	size_t total_size = x_min(x_ringbuf_free_size(rb), size);
	size_t front_buf_size = x_min(rb->front, total_size);
	memcpy(rb->buf + rb->front - front_buf_size, p + size - front_buf_size, front_buf_size);
	memcpy(rb->buf + rb->size - (total_size - front_buf_size), p + size - total_size, total_size - front_buf_size);
	rb->front = (rb->front + rb->size - total_size) % rb->size;
	return total_size;
}

size_t x_ringbuf_read_back(x_ringbuf *rb, void *buf, size_t size)
{
	size_t total_size = x_min(x_ringbuf_used_size(rb), size);
	char *p = buf;
	if (buf) {
		size_t rear_data_size = x_min(rb->rear, total_size);
		size_t left_data_size = total_size - rear_data_size;
		memcpy(p + total_size - rear_data_size, rb->buf + rb->rear - rear_data_size, rear_data_size);
		memcpy(p, rb->buf + rb->size - left_data_size, left_data_size);
	}
	rb->rear = (rb->rear + rb->size- total_size) % rb->size;
	return total_size;
}

size_t x_ringbuf_read(x_ringbuf *rb, void *buf, size_t size)
{
	size_t total_size = x_min(x_ringbuf_used_size(rb), size);
	if (buf) {
		size_t front_data_size = x_min((rb->size - rb->front), total_size);
		memcpy(buf, rb->buf + rb->front, front_data_size);
		memcpy((uint8_t *)buf + front_data_size, rb->buf, total_size - front_data_size);
	}
	rb->front = (rb->front + total_size) % rb->size;
	return total_size;
}

void x_ringbuf_resize(x_ringbuf *rb, size_t size)
{
	assert(rb);
	assert(size > 0);
	assert(x_ringbuf_used_size(rb) <= size - 1);
	if (rb->front > rb->rear) {
		long shift = (long)size - (long)rb->size;
		memmove(rb->buf + rb->front + shift, rb->buf + rb->front, rb->size - rb->front);
		rb->front += shift;
	}
	else if (rb->rear > size - 1) {
		memmove(rb->buf, rb->buf + rb->front, rb->rear - rb->front);
		rb->rear -= rb->front;
		rb->front = 0;
	}
	rb->size = size;
}

void *x_ringbuf_linearize(x_ringbuf *rb)
{
	assert(rb);
	if (rb->front <= rb->rear)
		return rb->buf + rb->front;
	size_t size = (rb->rear + rb->size - rb->front) % rb->size;
	size_t left = 0, mid = rb->rear, right = rb->front, d = 0;
	while (1) {
		if (mid - left < rb->size - right) {
			size_t swp_len = mid - left;
			if (d % 2 == 1)
				memcpy(rb->buf + left, rb->buf + right, swp_len);
			else
				x_memswp(rb->buf + left, rb->buf + right, swp_len);
			left += swp_len;
			mid = right;
			right = mid + swp_len;
			d += 1;
		}
		else {
			size_t swp_len = rb->size - right;
			x_memswp(rb->buf + left, rb->buf + right, swp_len);
			left += swp_len;
			if (d % 2 == 1)
				break;
			d = 0;
		}
	}
	memcpy(rb->buf + left, rb->buf + mid, right - mid);
	rb->front = 0;
	rb->rear = size;
	return rb->buf + rb->front;
}

void x_ringbuf_compact(x_ringbuf *b)
{
	if (b->front <= b->rear)
		memmove(b->buf, b->buf + b->front, x_ringbuf_used_size(b));
	else
		x_ringbuf_linearize(b);
	b->rear = x_ringbuf_used_size(b);
	b->front = 0;
}

size_t x_ringbuf_pour(x_ringbuf *src, x_ringbuf *dst, size_t size)
{
	size_t moved_size = 0;
	size_t read_size, write_size;
	do {
		x_iovec iov = x_ringbuf_acquire_read(src);
		read_size = x_min(size - moved_size, iov.len);
		write_size = x_ringbuf_write(dst, iov.base, read_size);
		x_ringbuf_commit_read(src, write_size);
		moved_size += write_size;
	} while (write_size && moved_size != size);
	return moved_size;
}

size_t x_ringbuf_pour_backward(x_ringbuf *src, x_ringbuf *dst, size_t size)
{
	size_t moved_size = 0;
	size_t read_size, write_size;
	do {
		x_iovec iov = x_ringbuf_acquire_read_back(src);
		read_size = x_min(size - moved_size, iov.len);
		write_size = x_ringbuf_write_front(dst, (char *)iov.base + iov.len - read_size, read_size);
		x_ringbuf_commit_read_back(src, write_size);
		moved_size += write_size;
	} while (write_size && moved_size != size);
	return moved_size;
}

size_t x_ringbuf_copy(x_ringbuf *src, size_t start, size_t size, x_ringbuf *dst)
{
	x_ringbuf tmp = *src;
	x_ringbuf_read(&tmp, NULL, start);
	return x_ringbuf_pour(&tmp, dst, size);
}

size_t x_ringbuf_copy_backward(x_ringbuf *src, size_t start, size_t size, x_ringbuf *dst)
{
	x_ringbuf tmp = *src;
	x_ringbuf_read_back(&tmp, NULL, start);
	return x_ringbuf_pour(&tmp, dst, size);
}

size_t x_ringbuf_peek(x_ringbuf *rb, void *buf, size_t start, size_t size)
{
	assert(buf != NULL);
	x_ringbuf tmp = *rb;
	x_ringbuf_read(&tmp, NULL, start);
	return x_ringbuf_read(&tmp, buf, size);
}

size_t x_ringbuf_peek_backward(x_ringbuf *rb, void *buf, size_t start, size_t size)
{
	assert(buf != NULL);
	x_ringbuf tmp = *rb;
	x_ringbuf_read_back(&tmp, NULL, start);
	return x_ringbuf_read_back(&tmp, buf, size);
}

