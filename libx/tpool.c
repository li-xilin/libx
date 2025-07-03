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

#include "x/tpool.h"
#include "x/thread.h"
#include "x/mutex.h"
#include "x/cond.h"
#include <stdlib.h>
#include <string.h>

x_tpool_work *x_tpool_work_create(x_tpool_worker_f *func, void *arg)
{
	assert(func);
	x_tpool_work *work;
	work = malloc(sizeof *work);
	if (!work)
		return NULL;
	work->func = func;
	work->arg  = arg;
	work->next = NULL;
	return work;
}

void x_tpool_work_free(x_tpool_work *work)
{
	if (!work)
		return;
	free(work);
}

static x_tpool_work *x_tpool_work_get(x_tpool *tp)
{
	x_tpool_work *work = tp->work_first;
	assert(tp);
	if (work == NULL)
		return NULL;
	if (work->next == NULL) {
		tp->work_first = NULL;
		tp->work_last  = NULL;
	} else {
		tp->work_first = work->next;
	}
	return work;
}

static int tpool_worker(void)
{
	x_tpool *tp = x_thread_data();
	x_tpool_work *work;
	while (true) {
		x_mutex_lock(&(tp->work_mutex));
		while (tp->work_first == NULL && !tp->stop)
			x_cond_sleep(&(tp->work_cond), &(tp->work_mutex), -1);
		if (tp->stop)
			break;
		work = x_tpool_work_get(tp);
		x_mutex_unlock(&(tp->work_mutex));
		if (work != NULL) {
			work->func(work->arg);
			x_tpool_work_free(work);
		}
		x_mutex_lock(&(tp->work_mutex));
		tp->working_cnt--;
		if (!tp->stop && tp->working_cnt == 0 && tp->work_first == NULL)
			x_cond_wake(&(tp->working_cond));
		x_mutex_unlock(&(tp->work_mutex));
	}
	tp->thread_cnt--;
	x_cond_wake(&(tp->working_cond));
	x_mutex_unlock(&(tp->work_mutex));
	return 0;
}

int x_tpool_init(x_tpool *tp, size_t num)
{
	x_thread **thds = NULL;
	if (num == 0)
		num = 2;
	memset(tp, 0, sizeof *tp);
	tp->thread_cnt = num;
	x_mutex_init(&tp->work_mutex);
	x_cond_init(&tp->work_cond);
	x_cond_init(&tp->working_cond);
	thds = malloc(sizeof(x_thread *) * num);
	if (!thds)
		return -1;
	for (int i = 0; i < num; i++) {
		thds[i] = x_thread_create(tpool_worker, NULL, tp);
		if (!thds[i]) {
			for (int j = i - 1; j >= 0; j++) {
				x_thread_kill(thds[j]);
				x_thread_join(thds[j], NULL);
			}
			free(thds);
			return -1;
		}
	}
	tp->thread_list = thds;
	return 0;
}

void x_tpool_destroy(x_tpool *tp)
{
	if (!tp)
		return;
	x_tpool_work *work;
	x_tpool_work *work2;
	x_mutex_lock(&(tp->work_mutex));
	work = tp->work_first;
	while (work != NULL) {
		work2 = work->next;
		x_tpool_work_free(work);
		work = work2;
	}
	tp->stop = true;
	x_cond_wake_all(&(tp->work_cond));
	x_mutex_unlock(&(tp->work_mutex));
	x_tpool_wait(tp);
	for (int i = 0; i < tp->thread_cnt; i++) {
		x_thread_kill(tp->thread_list[i]);
		x_thread_join(tp->thread_list[i], NULL);
	}
	x_mutex_destroy(&(tp->work_mutex));
	x_cond_destroy(&(tp->work_cond));
	x_cond_destroy(&(tp->working_cond));
}

void x_tpool_add_work(x_tpool *tp, x_tpool_work *work)
{
	assert(tp);
	assert(work);
	x_mutex_lock(&(tp->work_mutex));
	tp->working_cnt++;
	if (!tp->work_first) {
		tp->work_first = work;
		tp->work_last = tp->work_first;
	}
	else {
		tp->work_last->next = work;
		tp->work_last = work;
	}
	x_cond_wake_all(&(tp->work_cond));
	x_mutex_unlock(&(tp->work_mutex));
}

void x_tpool_wait(x_tpool *tp)
{
	assert(tp);
	x_mutex_lock(&(tp->work_mutex));
	while (true) {
		if ((!tp->stop && tp->working_cnt != 0) || (tp->stop && tp->thread_cnt != 0))
			x_cond_sleep(&(tp->working_cond), &(tp->work_mutex), -1);
		else
			break;
	}
	x_mutex_unlock(&(tp->work_mutex));
}

