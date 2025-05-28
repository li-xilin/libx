/*
 * Copyright (c) 2021-2024 Li Xilin <lixilin@gmx.com>
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

#ifndef X_THREAD_H
#define X_THREAD_H

#include "types.h"
#include "assert.h"
#include "detect.h"
#include <errno.h>
#include <assert.h>
#include <stdlib.h>
#include <stdint.h>

#ifdef X_OS_WIN
#include <windows.h>
#include <process.h>
struct x_thread_st
{
	HANDLE hThread;
};
#else
#include <pthread.h>
#include <unistd.h>
#include <limits.h>
#undef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 199309L
#include <time.h>
struct x_thread_st
{
	pthread_t pthread;
};
#endif

typedef uintptr_t (x_thread_func_f)(void *arg);

static inline x_thread x_thread_self(void)
{
#ifdef X_OS_WIN
	return (x_thread) { .hThread = GetCurrentThread() };
#else
        return (x_thread) { .pthread = pthread_self() };
#endif
}

struct __x_thread_argument_st
{
        void *arg;
        x_thread_func_f *func;
};

#ifdef X_OS_WIN
static DWORD WINAPI __x_thread_proc(void *arg)
{
	struct __x_thread_argument_st ta =
		*(struct __x_thread_argument_st *)arg;
	free(arg);
	DWORD dwRetCode = ta.func(ta.arg);
	void __x_tss_free_all_win32(void);
	__x_tss_free_all_win32();
	return dwRetCode;
}
#else
static void * __x_thread_proc(void *arg)
{
	struct __x_thread_argument_st ta =
		*(struct __x_thread_argument_st *)arg;
	free(arg);
	pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);
	pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
	return (void *)ta.func(ta.arg);
}
#endif

static inline x_thread *x_thread_create(x_thread_func_f *thread_func, void *arg)
{
	assert(thread_func);

	struct __x_thread_argument_st *ta = NULL;
	x_thread *t = NULL;

	if (!(t = malloc(sizeof *t)))
		goto fail;
	if (!(ta = malloc(sizeof *ta)))
		goto fail;

	ta->arg = arg;
	ta->func = thread_func;

#ifdef X_OS_WIN
	DWORD dwThreadId;
	t->hThread = CreateThread(NULL, 0, __x_thread_proc, ta, 0, &dwThreadId);
	if (!t->hThread)
		goto fail;
#else
	if (pthread_create(&t->pthread, NULL, __x_thread_proc, ta))
		goto fail;
#endif
	return t;
fail:
	free(t);
	free(ta);
	return NULL;
}

static inline void x_thread_exit(uintptr_t ret_code)
{
#ifdef X_OS_WIN
	void __x_tss_free_all_win32(void);
	__x_tss_free_all_win32();
	ExitThread(ret_code);
#else
	pthread_exit((void *)ret_code);
#endif
}

static inline int x_thread_sleep(unsigned int millise)
{
#ifdef X_OS_WIN
	x_assert(millise < UINT_MAX / 1000, "millise is too big");
	Sleep(millise);
	return 0;
#else
	struct timespec req;
	struct timespec rem;

	req.tv_sec = millise / 1000;
	req.tv_nsec = (millise % 1000) * 1000000;

	while (nanosleep(&req, &rem) == -1) {
		if (errno == EINTR)
			req = rem;
		else
			return -1;
	}
	return 0;
#endif
}

static inline int x_thread_join(x_thread *t, uintptr_t *ret_code)
{
	assert(t);
#ifdef X_OS_WIN
	DWORD dwRetCode = -1;
	if(WaitForSingleObject(t->hThread, INFINITE))
		return -1;

	if (!GetExitCodeThread(t->hThread, &dwRetCode))
		return -1;

	if (ret_code)
		*ret_code = dwRetCode;
	(void)CloseHandle(t->hThread);
#else
	void *retptr;
	int err = pthread_join(t->pthread, &retptr);
	if(err) {
		return -1;
	}
	if (retptr == PTHREAD_CANCELED)
		return -1;

	if (ret_code)
		*ret_code = (uintptr_t)retptr;
#endif
	free(t);
	return 0;
}

static inline int x_thread_detach(x_thread *t)
{
	assert(t);
#ifdef X_OS_WIN
	int retval = CloseHandle(t->hThread);
#else
	int retval = - !!pthread_detach(t->pthread);
#endif
	free(t);
	return retval;
}

static inline void x_thread_yield(void)
{
#ifdef X_OS_WIN
	SwitchToThread();
#else
	extern int sched_yield(void);
	sched_yield();
#endif
}

static inline void x_thread_kill(x_thread *t)
{
#ifdef X_OS_WIN
	(void)TerminateThread(t->hThread, 0);
#else
	(void)pthread_cancel(t->pthread);
#endif
}

#endif

