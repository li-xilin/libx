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
#include "x/deqbuf.h"
#include "x/string.h"
#include <stdlib.h>

#define PTR_SIZE (sizeof(void *))
#define BUCKET_COUNT(map) ((map)->size == 1 ? 16 : ((((map)->size - 1) / PTR_SIZE) << 1))

typedef size_t slice_handler_fn(char *slice, size_t slice_size, void *user);

void x_deqbuf_init1(x_deqbuf *deq, size_t seg_size, size_t segs_max)
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

void x_deqbuf_init(x_deqbuf *deq)
{
	x_deqbuf_init1(deq, X_DEQBUF_SEGMENT_SIZE, SIZE_MAX / X_DEQBUF_SEGMENT_SIZE);
}

void x_deqbuf_free(x_deqbuf *deq)
{
	x_cond_destroy(&deq->cons_cond);
	x_cond_destroy(&deq->prod_cond);
	x_mutex_destroy(&deq->lock);
	x_mset_free(&deq->mset);
}

size_t x_deqbuf_max_size(x_deqbuf *deq)
{
	return deq->segs_max * deq->seg_size;
}

static void map_realloc(x_deqbuf *deq, size_t bucket_cnt)
{
	x_deqbuf b = X_DEQBUF_INIT(b);
	size_t new_size = bucket_cnt * sizeof(void*) + 1;
	x_ringbuf tmp = X_RINGBUF_INIT(x_malloc(&deq->mset, new_size), new_size);
	x_ringbuf_pour(&deq->map, &tmp, x_ringbuf_used_size(&deq->map));
	x_free(deq->map.buf);
	deq->map = tmp;
}

static inline size_t seg_count(x_deqbuf *deq)
{
	return x_ringbuf_used_size(&deq->map) / PTR_SIZE;
}

static inline void *last_segment(x_deqbuf *deq)
{
	void *seg;
	size_t used_size = x_ringbuf_used_size(&deq->map);
	if (used_size == 0)
		return NULL;
	x_ringbuf_peek(&deq->map, &seg, used_size - PTR_SIZE, PTR_SIZE);
	return seg;
}

static inline void *first_segment(x_deqbuf *deq)
{
	void *seg;
	if (x_ringbuf_peek(&deq->map, &seg, 0, PTR_SIZE) == 0)
		return NULL;
	return seg;
}

static inline void *locate_front_segment(x_deqbuf *deq, size_t shift)
{
	void *seg;
	size_t used_size = x_ringbuf_used_size(&deq->map);
	if (used_size == 0)
		return NULL;
	if (x_ringbuf_peek(&deq->map, &seg, shift * PTR_SIZE, PTR_SIZE) == 0)
		return NULL;
	return seg;
}

#if 0
static inline void *locate_back_segment(x_deqbuf *deq, size_t shift)
{
	void *seg;
	size_t used_size = x_ringbuf_used_size(&deq->map);
	if (used_size == 0)
		return NULL;
	if (x_ringbuf_peek_backward(&deq->map, &seg, shift * PTR_SIZE, PTR_SIZE) == 0)
		return NULL;
	return seg;
}
#endif


static inline void *add_back_segment(x_deqbuf *deq)
{
	if (seg_count(deq) == deq->segs_max)
		return NULL;
	if (!deq->map.buf || x_ringbuf_free_size(&deq->map) == 0)
		map_realloc(deq, BUCKET_COUNT(&deq->map) << 1);
	void *seg = deq->free_cnt
		? deq->free_list[--deq->free_cnt]
		: x_zalloc(&deq->mset, deq->seg_size);
	x_ringbuf_write(&deq->map, &seg, PTR_SIZE);
	deq->rear = 0;
	return seg;
}

static inline void *add_front_segment(x_deqbuf *deq)
{
	if (seg_count(deq) == deq->segs_max)
		return NULL;
	if (!deq->map.buf || x_ringbuf_free_size(&deq->map) == 0)
		map_realloc(deq, BUCKET_COUNT(&deq->map) << 1);
	void *seg = deq->free_cnt
		? deq->free_list[--deq->free_cnt]
		: x_zalloc(&deq->mset, deq->seg_size);
	x_ringbuf_write_front(&deq->map, &seg, PTR_SIZE);
	deq->front = deq->seg_size;
	return seg;
}

static inline void del_front_segment(x_deqbuf *deq)
{
	void *seg;
	if (!x_ringbuf_read(&deq->map, &seg, PTR_SIZE))
		abort();
	if (deq->free_cnt != X_DEQBUF_FREE_LIMITS)
		deq->free_list[deq->free_cnt++] = seg;
	else
		x_free(seg);
	deq->front = 0;
}

size_t deqbuf_size(x_deqbuf *deq)
{
	size_t size = 0;
	size_t seg_cnt = x_ringbuf_used_size(&deq->map) / PTR_SIZE;
	if (!seg_cnt)
		goto out;
	size = (seg_cnt * deq->seg_size) - deq->front - (deq->seg_size - deq->rear);
out:
	return size;
}

static inline bool deqbuf_is_empty(x_deqbuf *deq)
{
	return x_ringbuf_is_empty(&deq->map);
}

static inline bool deqbuf_is_full(x_deqbuf *deq)
{
	return deqbuf_size(deq) / deq->seg_size == deq->segs_max;
}

static inline void del_back_segment(x_deqbuf *deq)
{
	void *seg;
	if (!x_ringbuf_read_back(&deq->map, &seg, PTR_SIZE))
		abort();
	if (deq->free_cnt != X_DEQBUF_FREE_LIMITS)
		deq->free_list[deq->free_cnt++] = seg;
	else
		x_free(seg);
	deq->rear = deq->seg_size;
}

static size_t slice_write_back(x_deqbuf *deq, slice_handler_fn *slice_handler, void *user)
{
	size_t moved_size;
	char *last_seg = last_segment(deq);
	size_t total_write = 0;
	if (!last_seg) {
		last_seg = add_back_segment(deq);
		if (!last_seg)
			goto out;
		deq->front = 0;
	}
	size_t tail_size = deq->seg_size - deq->rear;
	if (tail_size != 0) {
		total_write += moved_size = slice_handler(last_seg + deq->rear, tail_size, user);
		deq->rear += moved_size;
		if (moved_size < tail_size)
			goto out;
	}
	do {
		last_seg = add_back_segment(deq);
		if (!last_seg)
			goto out;
		total_write += moved_size = slice_handler(last_seg, deq->seg_size, user);
		deq->rear += moved_size;
	} while (moved_size == deq->seg_size);
out:
	return total_write;
}

static size_t slice_write_front(x_deqbuf *deq, slice_handler_fn *slice_handler, void *user)
{
	size_t moved_size;
	char *first_seg = first_segment(deq);
	size_t total_write = 0;
	if (!first_seg) {
		first_seg = add_front_segment(deq);
		if (!first_seg)
			goto out;
		deq->rear = deq->seg_size;
	}
	size_t head_size = deq->front;
	if (head_size != 0) {
		total_write += moved_size = slice_handler(first_seg, deq->front, user);
		deq->front -= moved_size;
		if (moved_size < head_size)
			goto out;
	}
	do {
		first_seg = add_front_segment(deq);
		if (!first_seg)
			goto out;
		total_write += moved_size = slice_handler(first_seg, deq->seg_size, user);
		deq->front -= moved_size;
	} while (moved_size == deq->seg_size);

out:
	return total_write;
}

static size_t slice_read_front(x_deqbuf *deq, slice_handler_fn *slice_handler, void *user)
{
	size_t moved_size;
	size_t total_read = 0;
	char *cur_seg = first_segment(deq), *last_seg = last_segment(deq);
	if (!cur_seg)
		goto out;
	if (cur_seg != last_seg) {
		size_t head_size = deq->seg_size - deq->front;
		total_read += moved_size = slice_handler(cur_seg + deq->front, head_size, user);
		deq->front += moved_size;
		if (moved_size < head_size)
			goto out;
		del_front_segment(deq);
		while ((cur_seg = first_segment(deq)) != last_seg) {
			total_read += moved_size = slice_handler(cur_seg, deq->seg_size, user);
			deq->front += moved_size;
			if (moved_size < deq->seg_size)
				goto out;
			del_front_segment(deq);
		}
	}
	size_t tail_size = deq->rear - deq->front;
	total_read += moved_size = slice_handler(cur_seg + deq->front, tail_size, user);
	deq->front += moved_size;
	if (moved_size < tail_size)
		goto out;
	del_front_segment(deq);
out:
	return total_read;
}


static size_t slice_read_back(x_deqbuf *deq, slice_handler_fn *slice_handler, void *user)
{
	size_t moved_size;
	size_t total_read = 0;
	char *cur_seg = last_segment(deq), *first_seg = first_segment(deq);
	if (!cur_seg)
		goto out;
	if (cur_seg != first_seg) {
		size_t tail_size = deq->rear;
		total_read += moved_size = slice_handler(cur_seg, tail_size, user);
		deq->rear -= moved_size;
		if (moved_size < tail_size)
			goto out;
		del_back_segment(deq);
		while ((cur_seg = last_segment(deq)) != first_seg) {
			total_read += moved_size = slice_handler(cur_seg, deq->seg_size, user);
			deq->rear -= moved_size;
			if (moved_size < deq->seg_size)
				goto out;
			del_back_segment(deq);
		}
	}
	size_t head_size = deq->rear - deq->front;
	total_read += moved_size = slice_handler(cur_seg + deq->front, head_size, user);
	deq->rear -= moved_size;
	if (moved_size < head_size)
		goto out;
	del_back_segment(deq);
out:
	return total_read;
}

struct transfer_ctx
{
	x_deqbuf *deq;
	char *data;
	size_t size;
	size_t ready_size;
};


static size_t put_forward(char *slice, size_t slice_size, void *user)
{
	struct transfer_ctx *ctx = user;
	size_t len = x_memcopy(slice, slice_size, ctx->data + ctx->ready_size, ctx->size - ctx->ready_size);
	ctx->ready_size += len;
	return len;
}

static size_t put_backward(char *slice, size_t slice_size, void *user)
{
	struct transfer_ctx *ctx = user;
	size_t len = x_memrcopy(slice, slice_size, ctx->data, ctx->size - ctx->ready_size);
	ctx->ready_size += len;
	return len;
}

static size_t take_backward(char *slice, size_t slice_size, void *user)
{
	struct transfer_ctx *ctx = user;
	size_t len = x_memrcopy(ctx->data, ctx->size - ctx->ready_size, slice, slice_size);
	ctx->ready_size += len;
	return len;
}

static size_t take_forwardr(char *slice, size_t slice_size, void *user)
{
	struct transfer_ctx *ctx = user;
	size_t len = x_memcopy(ctx->data + ctx->ready_size, ctx->size - ctx->ready_size, slice, slice_size);
	ctx->ready_size += len;
	return len;
}

size_t x_deqbuf_push(x_deqbuf *deq, const void *data, size_t size)
{
	struct transfer_ctx ctx = {
		.deq = deq,
		.data = (char *)data,
		.size = size,
	};
	x_mutex_lock(&deq->lock);
	while (deqbuf_is_full(deq))
		x_cond_sleep(&deq->prod_cond, &deq->lock, -1);
	size_t total_write = slice_write_back(deq, put_forward, &ctx);
	x_cond_wake_all(&deq->cons_cond);
	x_mutex_unlock(&deq->lock);
	return total_write;
}

size_t x_deqbuf_push_front(x_deqbuf *deq, const void *data, size_t size)
{
	struct transfer_ctx ctx = {
		.deq = deq,
		.data = (char *)data,
		.size = size,
	};
	x_mutex_lock(&deq->lock);
	while (deqbuf_is_full(deq))
		x_cond_sleep(&deq->prod_cond, &deq->lock, -1);
	size_t total_write = slice_write_front(deq, put_backward, &ctx);
	x_cond_wake_all(&deq->cons_cond);
	x_mutex_unlock(&deq->lock);
	return total_write;
}

size_t x_deqbuf_pop(x_deqbuf *deq, void *data, size_t size)
{
	struct transfer_ctx ctx = {
		.deq = deq,
		.data = data,
		.size = size,
	};
	x_mutex_lock(&deq->lock);
	while (deqbuf_is_empty(deq))
		x_cond_sleep(&deq->cons_cond, &deq->lock, -1);
	size_t total_read = slice_read_front(deq, take_forwardr, &ctx);
	x_cond_wake_all(&deq->prod_cond);
	x_mutex_unlock(&deq->lock);
	return total_read;
}

size_t x_deqbuf_pop_back(x_deqbuf *deq, void *data, size_t size)
{
	struct transfer_ctx ctx = {
		.deq = deq,
		.data = data,
		.size = size,
	};
	x_mutex_lock(&deq->lock);
	while (deqbuf_is_empty(deq))
		x_cond_sleep(&deq->cons_cond, &deq->lock, -1);
	size_t total_read = slice_read_back(deq, take_backward, &ctx);
	x_cond_wake_all(&deq->prod_cond);
	x_mutex_unlock(&deq->lock);
	return total_read;
}

void x_deqbuf_clear(x_deqbuf *deq)
{
	x_mutex_lock(&deq->lock);
	for (size_t i = 0; i < deq->map.size - 1; i += PTR_SIZE) {
		void *segment = NULL;
		x_ringbuf_peek(&deq->map, &segment, i, PTR_SIZE);
		x_free(segment);
	}
	x_ringbuf_clear(&deq->map);
	x_cond_wake_all(&deq->prod_cond);
	x_mutex_unlock(&deq->lock);
}

size_t x_deqbuf_size(x_deqbuf *deq)
{
	x_mutex_lock(&deq->lock);
	size_t size = deqbuf_size(deq);
	x_mutex_unlock(&deq->lock);
	return size;
}

bool x_deqbuf_is_empty(x_deqbuf *deq)
{
	x_mutex_lock(&deq->lock);
	bool is_empty = deqbuf_is_empty(deq);
	x_mutex_unlock(&deq->lock);
	return is_empty;
}

bool x_deqbuf_is_full(x_deqbuf *deq)
{
	x_mutex_lock(&deq->lock);
	bool is_full = deqbuf_is_full(deq);
	x_mutex_unlock(&deq->lock);
	return is_full;
}

size_t slice_walk_forward(x_deqbuf *deq, size_t start, slice_handler_fn *handler, void *user)
{
	size_t total_walk = 0;
	if (deqbuf_is_empty(deq))
		goto out;
	if (start >= deqbuf_size(deq))
		goto out;
	size_t seg_cnt = seg_count(deq);
	size_t seg_start, seg_front;
	size_t head_size = deq->seg_size - deq->front;
	if (seg_cnt == 1) {
		seg_start = 0;
		seg_front = x_min(deq->front + start, deq->rear);
	}
	else if (start < head_size) {
		seg_start = 0;
		seg_front = deq->front + start;
	}
	else {
		seg_start = 1 + (start - head_size) / deq->seg_size;
		seg_front = (start - head_size) % deq->seg_size;
	}
	for (size_t i = seg_start; i < seg_cnt; i++) {
		size_t front = i == seg_start ? seg_front : 0;
		size_t rear = i == seg_cnt - 1 ? deq->rear : deq->seg_size;
		size_t slice_size = rear - front;
		char *seg = locate_front_segment(deq, i);
		size_t walk_size = handler(seg + front, slice_size, user);
		total_walk += walk_size;
		if (walk_size < slice_size)
			break;
	}
out:
	return total_walk;
}

size_t slice_walk_backward(x_deqbuf *deq, size_t start, slice_handler_fn *handler, void *user)
{
	size_t total_walk = 0;
	if (deqbuf_is_empty(deq))
		goto out;
	if (start >= deqbuf_size(deq))
		goto out;
	size_t seg_cnt = seg_count(deq);
	size_t seg_end, seg_rear;
	size_t tail_size = deq->rear;
	if (seg_cnt == 1) {
		seg_end = seg_cnt;
		seg_rear = deq->rear - x_min(deq->rear - deq->front, start);
	}
	else if (start < tail_size) {
		seg_end = seg_cnt;
		seg_rear = deq->rear - start;
	}
	else {
		seg_end = seg_cnt - 1 - (start - tail_size) / deq->seg_size;
		seg_rear = deq->seg_size - (start - tail_size) % deq->seg_size;
	}
	for (size_t i = seg_end - 1; i != (size_t)-1; i--) {
		size_t rear = i == seg_end - 1 ? seg_rear : deq->seg_size;
		size_t front = i == 0 ? deq->front : 0;
		size_t slice_size = rear - front;
		char *seg = locate_front_segment(deq, i);
		size_t walk_size = handler(seg + front, slice_size, user);
		total_walk += walk_size;
		if (walk_size < slice_size)
			break;
	}
out:
	return total_walk;
}

size_t x_deqbuf_peek(x_deqbuf *deq, size_t start, void *buf, size_t size)
{
	struct transfer_ctx ctx = {
		.deq = deq,
		.data = buf,
		.size = size,
	};
	x_mutex_lock(&deq->lock);
	size_t total_read = slice_walk_forward(deq, start, take_forwardr, &ctx);
	x_mutex_unlock(&deq->lock);
	return total_read;
}

size_t x_deqbuf_peek_backward(x_deqbuf *deq, size_t start, void *buf, size_t size)
{
	struct transfer_ctx ctx = {
		.deq = deq,
		.data = buf,
		.size = size,
	};
	x_mutex_lock(&deq->lock);
	size_t total_read = slice_walk_backward(deq, start, take_backward, &ctx);
	x_mutex_unlock(&deq->lock);
	return total_read;
}


size_t x_deqbuf_modify(x_deqbuf *deq, size_t start, const void *data, size_t size)
{
	struct transfer_ctx ctx = {
		.deq = deq,
		.data = (char *)data,
		.size = size,
	};
	x_mutex_lock(&deq->lock);
	size_t total_read = slice_walk_forward(deq, start, put_forward, &ctx);
	x_mutex_unlock(&deq->lock);
	return total_read;

}

size_t x_deqbuf_modify_backward(x_deqbuf *deq, size_t start, const void *data, size_t size)
{
	struct transfer_ctx ctx = {
		.deq = deq,
		.data = (char *)data,
		.size = size,
	};
	x_mutex_lock(&deq->lock);
	size_t total_read = slice_walk_backward(deq, start, put_backward, &ctx);
	x_mutex_unlock(&deq->lock);
	return total_read;
}
