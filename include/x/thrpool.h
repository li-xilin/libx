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

#ifndef X_THRPOOL_H
#define X_THRPOOL_H

#include "linkage.h"
#include "types.h"
#include "cond.h"
#include "mutex.h"
#include "thread.h"
#include "memory.h"

X_EXTERN_C_BEGIN

typedef void x_thrwork_handler(x_thrwork *work);

struct x_thrpool_st
{
	x_mset mset;
	x_cond work_cond, complete_cond;
	size_t thread_cnt, working_cnt;
	x_mutex work_mutex;
	x_thread **thread_list;
	x_list work_list;
};

struct x_thrwork_st
{
	x_link link;
	x_thrwork_handler *handler;
};

int x_thrpool_init(x_thrpool *pool, size_t thr_cnt);
void x_thrpool_free(x_thrpool *pool);
void x_thrpool_add_work(x_thrpool *pool, x_thrwork *work);
void x_thrpool_remove_work(x_thrpool *pool, x_thrwork *work);
void x_thrpool_clear_work(x_thrpool *pool);
void x_thrpool_sync(x_thrpool *pool);

static inline void x_thrwork_init(x_thrwork *work, x_thrwork_handler *handler)
{
	work->handler = handler;
	x_link_init(&work->link);
}

X_EXTERN_C_END

#endif


