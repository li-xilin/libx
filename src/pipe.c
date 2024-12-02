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

#include "x/def.h"
#include "x/string.h"
#include "x/pipe.h"
#include <errno.h>

size_t x_pipe_write(x_pipe *pipe, void *p, size_t size)
{
        assert(p != NULL);
        size_t buf_size = x_min(x_pipe_buffer_size(pipe), size);
        size_t size1 = x_min(pipe->size - pipe->rear, buf_size);
        memcpy(pipe->buf + pipe->rear, p, size1);
        memcpy(pipe->buf, (uint8_t *)p + size1, buf_size - size1);
        pipe->rear = (pipe->rear + buf_size) % pipe->size;
        return buf_size;
}

size_t x_pipe_peek(x_pipe *pipe, void *buf, size_t start, size_t size)
{
        assert(buf != NULL);

        size_t data_size = x_pipe_data_size(pipe);
        if (start >= data_size)
                return 0;

        size_t read_size = x_min(x_pipe_data_size(pipe) - start, size);
        size_t new_front = (pipe->front + start) % pipe->size;
        size_t size1 = x_min(pipe->size - new_front, read_size);

        memcpy(buf, pipe->buf + new_front, size1);
        memcpy((uint8_t *)buf + size1, pipe->buf, read_size - size1);
        return read_size;
}

size_t x_pipe_read(x_pipe *pipe, void *buf, size_t size)
{
        size_t read_size = x_min(x_pipe_data_size(pipe), size);
        if (buf) {
                size_t size1 = x_min((pipe->size - pipe->front), read_size);
                memcpy(buf, pipe->buf + pipe->front, size1);
                memcpy((uint8_t *)buf + size1, pipe->buf, read_size - size1);
        }
        pipe->front = (pipe->front + read_size) % pipe->size;
        return read_size;
}

void *x_pipe_change_buffer(x_pipe *pipe, void *buf, size_t size)
{
	assert(pipe);
	assert(buf);
	assert(size > 1);

        if (x_pipe_data_size(pipe) < size - 1) {
                errno = EINVAL;
                return NULL;
        }

        if (pipe->front <= pipe->rear)
                memcpy(buf, pipe->buf + pipe->front, pipe->rear - pipe->front);
        else {
                memcpy(buf, pipe->buf + pipe->front, pipe->size - pipe->front);
                memcpy((uint8_t *)buf + pipe->size - pipe->front, pipe->buf , pipe->rear);
        }

        pipe->buf = buf;
        pipe->rear = x_pipe_data_size(pipe);
        pipe->front = 0;
        return pipe->buf;
}

size_t x_pipe_drain(x_pipe *pipe, size_t size, x_pipe_drain_f cb, void *arg)
{
	assert(pipe);
	assert(cb);

        size_t read_size = x_min(x_pipe_data_size(pipe), size);
        size_t size1 = x_min((pipe->size - pipe->front), read_size);
        if (size1)
                cb(pipe->buf + pipe->front, size1, arg);
        if (read_size - size1)
                cb(pipe->buf, read_size - size1, arg);
        pipe->front = (pipe->front + read_size) % pipe->size;
        return read_size;
}

void *x_pipe_pullup(x_pipe *pipe)
{
	assert(pipe);

        if (pipe->front <= pipe->rear)
                return pipe->buf + pipe->front;

        size_t size = (pipe->rear + pipe->size - pipe->front) % pipe->size;
        size_t left = 0, mid = pipe->rear, right = pipe->front, d = 0;

        while (1) {
                if (mid - left < pipe->size - right) {
                        size_t swp_len = mid - left;
                        if (d % 2 == 1)
                                memcpy(pipe->buf + left, pipe->buf + right, swp_len);
                        else
                                x_memswp(pipe->buf + left, pipe->buf + right, swp_len);
                        left += swp_len;
                        mid = right;
                        right = mid + swp_len;
                        d += 1;
                }
                else {
                        size_t swp_len = pipe->size - right;
                        x_memswp(pipe->buf + left, pipe->buf + right, swp_len);
                        left += swp_len;
                        if (d % 2 == 1)
                                break;
                        d = 0;
                }

        }
        memcpy(pipe->buf + left, pipe->buf + mid, right - mid);
        pipe->front = 0;
        pipe->rear = size;
        return pipe->buf + pipe->front;
}

size_t x_pipe_pour(x_pipe *src, x_pipe *dst, size_t size)
{
	size_t moved_size = 0;
	size_t read_size, write_size;

	do {
		read_size = x_min(size - moved_size, x_pipe_zread_size(src));
		write_size = x_pipe_write(dst, x_pipe_zread(src), read_size);
		x_pipe_zread_commit(src, write_size);
		moved_size += write_size;
	} while (read_size && write_size);
	return moved_size;
}

