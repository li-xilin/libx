/*
 * Copyright (c) 2022-2024 Li Xilin <lixilin@gmx.com>
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

#ifndef X_TPOOL_H
#define X_TPOOL_H

#include "x/types.h"
#include "x/cond.h"
#include "x/thread.h"

typedef void x_tpool_worker_f(void *arg);

struct x_tpool_st
{
	x_tpool_work *work_first, *work_last;
	x_cond work_cond, working_cond;
	size_t working_cnt, thread_cnt;
	x_mutex work_mutex;
	x_thread **thread_list;
	bool stop;
};

struct x_tpool_work_st
{
	x_tpool_worker_f *func;
	void *arg;
	struct x_tpool_work_st *next;
};

x_tpool_work *x_tpool_work_create(x_tpool_worker_f *func, void *arg);
void x_tpool_work_free(x_tpool_work *work);
int x_tpool_init(x_tpool *p, size_t num);
void x_tpool_destroy(x_tpool *tpool);
void x_tpool_add_work(x_tpool *tpool, x_tpool_work *work);
void x_tpool_wait(x_tpool *tpool);

#endif
