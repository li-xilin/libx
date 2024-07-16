/*
 * Copyright (c) 2023 Li Xilin <lixilin@gmx.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#ifndef X_ONCE_H
#define X_ONCE_H

#include "detect.h"
#include <errno.h>
#include <assert.h>

#ifdef X_OS_WIN
#include <synchapi.h>
struct x_once_st
{
	INIT_ONCE InitOnce;
};
#define X_ONCE_INIT { INIT_ONCE_STATIC_INIT }
#else
#include <pthread.h>
struct x_once_st
{
	pthread_once_t once;
};
#define X_ONCE_INIT { PTHREAD_ONCE_INIT }
#endif

#ifndef X_ONCE_DEFINED
#define X_ONCE_DEFINED
typedef struct x_once_st x_once;
#endif

typedef void x_once_f(void);

#ifdef X_OS_WIN
static BOOL CALLBACK __x_once_proc_win32(PINIT_ONCE InitOnce, PVOID pInitProc, PVOID *lpContext)
{
	((x_once_f *)(uintptr_t)pInitProc)();
	return TRUE;
}
#endif

static inline int x_once_run(x_once *once, x_once_f *once_proc)
{
#ifdef X_OS_WIN
	if (!InitOnceExecuteOnce(&once->InitOnce, __x_once_proc_win32, (PINIT_ONCE_FN *)(INT_PTR)once_proc, NULL)) {
		return -1;
	}
#else
	if (pthread_once(&once->once, once_proc)) {
		errno = EINVAL;
		return -1;
	}
#endif
	return 0;
}

#endif

