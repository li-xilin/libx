/*
 * Copyright (c) 2024,2025 Li Xilin <lixilin@gmx.com>
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

#include "x/thread.h"
#include "x/detect.h"
#include "x/tss.h"
#include "x/mutex.h"
#include "x/cond.h"
#include "x/once.h"

#ifdef X_OS_WIN
#  include <processthreadsapi.h>
#  include <process.h>
#  include <synchapi.h>
#  include <handleapi.h>
#  include <windef.h>
#  include <winbase.h>
#else
#  include <pthread.h>
#  include <unistd.h>
#  include <limits.h>
#  include <time.h>
#endif
#include <setjmp.h>

struct x_thread_st
{
#ifdef X_OS_WIN
	HANDLE hThread;
#else
	pthread_t pthread;
#endif
	void *data;
	x_thread_clean_fn *cleanup;
	jmp_buf jmp_exit;
	uint8_t detached:1, exited:1, canceled:1;
};

struct thread_info
{
	void *arg;
	x_thread_fn *func;
	x_thread *thread;
	x_cond finished_cond;
	x_mutex mutex;
	int finished, error;
};

static x_once s_thread_once = X_ONCE_INIT;
static x_tss s_thread_ptr;
static int s_init_errno = 0;
static x_mutex s_lock;

static void free_thread_tss(void *data)
{
}

static void init(void)
{
	s_init_errno = 0;
	x_mutex_init(&s_lock);
	if (x_tss_init(&s_thread_ptr, free_thread_tss)) {
		s_init_errno = errno;
		x_mutex_destroy(&s_lock);
		return;
	}
}

uint32_t x_thread_native_id(void)
{
#ifdef X_OS_WIN
	return GetCurrentThreadId();
#elif defined(X_OS_LINUX)
#define PTHREAD_OFFSETOF_TID 0x2d0
	return *(uint32_t *)(((char *)pthread_self()) + PTHREAD_OFFSETOF_TID);
#else
	return (uint32_t)pthread_self();
#endif
}

x_thread *x_thread_self(void)
{
	return  x_tss_get(&s_thread_ptr);
}

static int init_thread_info(struct thread_info *info)
{
	x_mutex_lock(&info->mutex);
	if (x_tss_set(&s_thread_ptr, info->thread))
		info->error = errno;
	info->finished = true;
	x_cond_wake(&info->finished_cond);
	x_mutex_unlock(&info->mutex);
	if (info->error)
		return -1;
	return 0;
}

static void free_thread(x_thread *thread)
{
	x_mutex_lock(&s_lock);
	if (thread->cleanup)
		thread->cleanup();
	if (thread->detached)
		free(thread);
	else
		thread->exited = 1;
	x_mutex_unlock(&s_lock);
}

#ifdef X_OS_WIN
static DWORD WINAPI __x_thread_proc(void *arg)
{
	struct thread_info *info = arg;
	x_thread *thread = info->thread;
	if (init_thread_info(info))
		return 0;
	DWORD dwRetCode = setjmp(info->thread->jmp_exit);
	BOOL bIsJump = FALSE;
	if (!bIsJump && dwRetCode == 0) {
		bIsJump = TRUE;
		dwRetCode = info->func();
	}
	void __x_tss_free_all_win32(void);
	__x_tss_free_all_win32();
	free_thread(thread);
	return dwRetCode;
}
#else
void thread_exit_unexpected(void *arg) {
	x_thread *thread = arg;
	longjmp(thread->jmp_exit, -1);
}

static void * __x_thread_proc(void *arg)
{
	struct thread_info *info = arg;
	x_thread *thread = info->thread;
	if (init_thread_info(info))
		return NULL;
	pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED, NULL);
	pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL);
	bool is_jmp = false;
	int retval;
	pthread_cleanup_push(thread_exit_unexpected, thread);
	retval = setjmp(info->thread->jmp_exit);;
	if (!is_jmp && retval == 0) {
		is_jmp = true;
		retval = info->func();
	}
	pthread_cleanup_pop(0);
	free_thread(thread);
	return (void *)(uintptr_t)retval;
}
#endif

x_thread *x_thread_create(x_thread_fn *thread_func, x_thread_clean_fn *cleanup, void *arg)
{
	struct thread_info *info = NULL;
	x_thread *t = NULL;

	assert(thread_func);

	x_once_init(&s_thread_once, init);
	if (s_init_errno) {
		errno = s_init_errno;
		return NULL;
	}
	if (!(t = calloc(1, sizeof *t)))
		goto fail;
	t->cleanup = cleanup;
	t->data = arg;
	if (!(info = calloc(1, sizeof *info)))
		goto fail;
	if (x_cond_init(&info->finished_cond))
		goto fail;
	x_mutex_init(&info->mutex);
	info->arg = arg;
	info->func = thread_func;
	info->thread = t;
#ifdef X_OS_WIN
	DWORD dwThreadId;
	t->hThread = CreateThread(NULL, 0, __x_thread_proc, info, 0, &dwThreadId);
	if (!t->hThread)
		goto fail;
#else
	if (pthread_create(&t->pthread, NULL, __x_thread_proc, info))
		goto fail;
#endif
	x_mutex_lock(&info->mutex);
	while (!info->finished)
		x_cond_sleep(&info->finished_cond, &info->mutex, -1);
	x_mutex_unlock(&info->mutex);

	if (info->error) {
		errno = info->error;
		x_cond_destroy(&info->finished_cond);
		x_mutex_destroy(&info->mutex);
		goto fail;
	}
	return t;
fail:
	free(t);
	free(info);
	return NULL;
}

int x_thread_sleep(unsigned int millise)
{
	x_assert(millise < UINT_MAX / 1000, "millise is too big");
#ifdef X_OS_WIN
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

int x_thread_join(x_thread *t, int *ret_code)
{
	assert(t);
	x_assert(!t->detached, "Thread already detached");
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
	if (retptr == PTHREAD_CANCELED) {
		errno = ECANCELED;
		retptr = (void *)-1;
	}

	if (ret_code)
		*ret_code = (uintptr_t)retptr;
#endif
	return 0;
}

int x_thread_free(x_thread *t)
{
	assert(t);
	x_assert(!t->detached, "Thread already detached");
#ifdef X_OS_WIN
	int retval = CloseHandle(t->hThread);
#else
	int retval = - !!pthread_detach(t->pthread);
#endif
	x_mutex_lock(&s_lock);
	if (t->exited)
		free(t);
	else
		t->detached = 1;
	x_mutex_unlock(&s_lock);
	return retval;
}

void x_thread_yield(void)
{
#ifdef X_OS_WIN
	SwitchToThread();
#else
	extern int sched_yield(void);
	sched_yield();
#endif
}

void x_thread_kill(x_thread *t)
{
	assert(t);
	x_assert(!t->detached, "Thread already detached");
#ifdef X_OS_WIN
	(void)TerminateThread(t->hThread, -1);
#else
	pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
	pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);
	(void)pthread_cancel(t->pthread);
#endif
}

void x_thread_testcancel(int retcode)
{
	x_thread *thread = x_thread_self();
	x_assert(thread, "Try to cancel the thread which not created by x_thread_create");
	if (thread->canceled)
		longjmp(thread->jmp_exit, retcode);
}

void x_thread_cancel(x_thread *t)
{
	assert(t);
	x_assert(!t->detached, "Thread already detached");
	t->canceled = 1;
}

void *x_thread_data(void)
{
	x_thread *thread = x_thread_self();
	x_assert(thread, "Try to cancel the thread which not created by x_thread_create");
	return thread->data;
}

void x_thread_cleanup(void)
{
#ifdef X_OS_WIN
	void __x_tss_free_all_win32(void);
	__x_tss_free_all_win32();
#endif
}
