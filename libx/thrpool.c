/*
 * Copyright (c) 2022-2025 Li Xilin <lixilin@gmx.com>
 * 
 * Permission is hereby granted, free of charge, to one person obtaining a copy
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

#include "x/thrpool.h"
#include "x/thread.h"
#include "x/mutex.h"
#include "x/cond.h"
#include <stdlib.h>
#include <string.h>

static int pool_worker(void)
{
	x_thrpool *pool = x_thread_data();
	x_thrwork *work;
	while (true) {
		x_mutex_lock(&(pool->work_mutex));
		while (x_list_is_empty(&pool->work_list)) {
			if (x_thread_canceled())
				goto out;
			x_cond_sleep(&pool->work_cond, &pool->work_mutex, -1);
		}
		work = x_container_of(x_list_first(&pool->work_list), x_thrwork, link);
		x_list_del(&work->link);
		pool->working_cnt++;

		x_thrwork_handler *handler = work->handler;
		x_mutex_unlock(&(pool->work_mutex));
		handler(work);
		x_mutex_lock(&(pool->work_mutex));

		pool->working_cnt--;
		if (pool->working_cnt == 0 && x_list_is_empty(&pool->work_list))
			x_cond_wake_all(&(pool->complete_cond));
		x_mutex_unlock(&(pool->work_mutex));
	}
out:
	x_mutex_unlock(&(pool->work_mutex));
	return 0;
}

int x_thrpool_init(x_thrpool *pool, size_t thr_cnt)
{
	assert(pool);
	x_thread **thr_list = NULL;
	thr_cnt = x_max(thr_cnt, 2);
	memset(pool, 0, sizeof *pool);
	pool->thread_cnt = thr_cnt;
	pool->working_cnt = 0;
	x_mset_init(&pool->mset);
	x_mutex_init(&pool->work_mutex);
	x_list_init(&pool->work_list);
	x_cond_init(&pool->work_cond);
	x_cond_init(&pool->complete_cond);
	thr_list = x_malloc(&pool->mset, sizeof(x_thread *) * thr_cnt);
	for (int i = 0; i < thr_cnt; i++) {
		thr_list[i] = x_thread_create(pool_worker, NULL, pool);
		if (!thr_list[i]) {
			for (int j = i - 1; j >= 0; j++) {
				x_thread_kill(thr_list[j]);
				x_thread_join(thr_list[j], NULL);
			}
			free(thr_list);
			return -1;
		}
	}
	pool->thread_list = thr_list;
	return 0;
}

static void clear_pending_works(x_thrpool *pool)
{
	x_list_popeach(cur, &pool->work_list);
}

void x_thrpool_clear_work(x_thrpool *pool)
{
	assert(pool);
	x_mutex_lock(&(pool->work_mutex));
	clear_pending_works(pool);
	x_mutex_unlock(&(pool->work_mutex));
}

void x_thrpool_free(x_thrpool *pool)
{
	if (!pool)
		return;
	x_thrpool_clear_work(pool);
	x_thrpool_sync(pool);
	x_mutex_lock(&(pool->work_mutex));
	for (int i = 0; i < pool->thread_cnt; i++)
		x_thread_cancel(pool->thread_list[i]);
	x_cond_wake_all(&(pool->work_cond));
	x_mutex_unlock(&(pool->work_mutex));
	for (int i = 0; i < pool->thread_cnt; i++)
		x_thread_join(pool->thread_list[i], NULL);
	x_mutex_destroy(&(pool->work_mutex));
	x_cond_destroy(&(pool->work_cond));
	x_cond_destroy(&(pool->complete_cond));
	x_mset_free(&pool->mset);
}

void x_thrpool_add_work(x_thrpool *pool, x_thrwork *work)
{
	assert(pool);
	assert(work);
	assert(!work->link.next);
	x_mutex_lock(&(pool->work_mutex));
	x_cond_wake(&(pool->work_cond));
	x_list_add_back(&pool->work_list, &work->link);
	x_mutex_unlock(&(pool->work_mutex));
}

void x_thrpool_remove_work(x_thrpool *pool, x_thrwork *work)
{
	assert(pool);
	assert(work);
	if (work->link.next) {
		x_mutex_lock(&(pool->work_mutex));
		x_list_del(&work->link);
		if (x_list_is_empty(&pool->work_list) && pool->working_cnt == 0)
			x_cond_wake_all(&(pool->complete_cond));
		x_mutex_unlock(&(pool->work_mutex));
	}
}

void x_thrpool_sync(x_thrpool *pool)
{
	assert(pool);
	x_mutex_lock(&(pool->work_mutex));
	while (!x_list_is_empty(&pool->work_list) || pool->working_cnt != 0)
		x_cond_sleep(&pool->complete_cond, &pool->work_mutex, -1);
	x_mutex_unlock(&(pool->work_mutex));
}

