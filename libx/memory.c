/*
 * Copyright (c) 2023,2025 Li Xilin <lixilin@gmx.com>
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

#include "x/list.h"
#include "x/memory.h"
#include "x/thread.h"
#include "x/mutex.h"
#include "x/once.h"
#include <stdlib.h>
#include <string.h>

#define RETRY_INTERVAL 20

static x_once s_mset_once = X_ONCE_INIT;
static x_mset s_mset = {
	.list = X_LIST_INIT(s_mset.list),
	.lock = X_MUTEX_INIT
};

static void init_mset(void)
{
	x_mutex_init(&s_mset.lock);
}

void x_mset_init(x_mset *mset)
{
#ifdef X_OS_WIN
	// Prevent infinite loop
	size_t size = sizeof(block_st) + sizeof *lock->section;
	lock->section = malloc(size);
	while (!lock->section) {
		x_thread_sleep(RETRY_INTERVAL);
		lock->section = malloc(size);
	}
	InitializeCriticalSection(lock->section);
	x_list_add_back(&mset->list, &b->link);
#endif
	x_list_init(&mset->list);
}

void x_mset_clear(x_mset *mset)
{
	if (!mset)
		mset = &s_mset;
	while (!x_list_is_empty(&mset->list)) {
		x_link *pos = x_list_first(&mset->list);
		x_list_del(pos);
		free(x_container_of(pos, struct block_st, link));
	}
}

void x_mset_free(x_mset *mset)
{
	if (!mset) {
		if (x_list_is_empty(&s_mset.list))
			return;
		x_once_init(&s_mset_once, init_mset);
		mset = &s_mset;
	}

	while (!x_list_is_empty(&mset->list)) {
		x_link *pos = x_list_first(&mset->list);
		x_list_del(pos);
		free(x_container_of(pos, struct block_st, link));
	}
	x_mutex_destroy(&mset->lock);
}

void *x_malloc(x_mset *mset, size_t size)
{
	struct block_st *b = malloc(sizeof *b + size);
	while (!b) {
		x_thread_sleep(RETRY_INTERVAL);
		b = malloc(sizeof *b + size);
	}
	if (!mset) {
		x_once_init(&s_mset_once, init_mset);
		mset = &s_mset;
	}
	b->mset = mset;
	x_mutex_lock(&b->mset->lock);
	x_list_add_back(&mset->list, &b->link);
	x_mutex_unlock(&b->mset->lock);
	return b->data;
}

void *x_zalloc(x_mset *mset, size_t size)
{
	void *p = x_malloc(mset, size);
	memset(p, 0, size);
	return p;
}

void *x_calloc(x_mset *mset, size_t nmemb, size_t size)
{
	void *p = x_malloc(mset, nmemb * size);
	memset(p, 0, nmemb * size);
	return p;
}

void *x_realloc(void *ptr, size_t size)
{
	if (!ptr)
		return x_malloc(NULL, size);
	struct block_st *b = x_container_of(ptr, struct block_st, data);
	assert(b->mset);
	x_mutex_lock(&b->mset->lock);
	x_list_del(&b->link);
	x_mutex_unlock(&b->mset->lock);
	struct block_st *new_blk = realloc(b, sizeof *b + size);
	if (new_blk)
		b = NULL;
	while (!new_blk) {
		x_thread_sleep(RETRY_INTERVAL);
		struct block_st **bp = &b;
		new_blk = realloc(*bp, sizeof *b + size);
	}
	if (new_blk->mset) {
		x_mutex_lock(&new_blk->mset->lock);
		x_list_add_back(&new_blk->mset->list, &new_blk->link);
		x_mutex_unlock(&new_blk->mset->lock);
	}
	return new_blk->data;
}

void x_free(void *ptr)
{
	if (!ptr)
		return;
	struct block_st *b = x_container_of(ptr, struct block_st, data);
	assert(b->mset);
	x_mutex_lock(&b->mset->lock);
	x_list_del(&b->link);
	x_mutex_unlock(&b->mset->lock);
	free(b);
}

void x_mdetach(void *ptr)
{
	struct block_st *b = x_container_of(ptr, struct block_st, data);
	if (b->mset) {
		x_mutex_lock(&b->mset->lock);
		b->mset = NULL;
		x_mutex_unlock(&b->mset->lock);
		x_list_del(&b->link);
		b->mset = NULL;
	}
}

void x_mattach(x_mset *mset, void *ptr)
{
	struct block_st *b = x_container_of(ptr, struct block_st, data);
	x_mdetach(ptr);
	b->mset = mset;
	if (!mset)
		mset = &s_mset;
	x_mutex_lock(&b->mset->lock);
	x_list_add_back(&mset->list, &b->link);
	x_mutex_unlock(&b->mset->lock);
}

