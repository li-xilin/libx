/*
 * Copyright (c) 2023-2024 Li Xilin <lixilin@gmx.com>
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

#ifndef X_MUTEX_H
#define X_MUTEX_H

#include "detect.h"
#include <errno.h>
#include <assert.h>

#ifdef X_OS_WIN
#include <synchapi.h>
#include <windows.h>
#define X_MUTEX_INIT { .section = NULL }
#else
#include <pthread.h>
#define X_MUTEX_INIT { .mutex = PTHREAD_MUTEX_INITIALIZER }
#endif

#ifndef X_THREAD_DEFINED
#define X_THREAD_DEFINED
typedef struct x_thread_st x_thread;
#endif

#ifndef X_MUTEX_DEFINED
#define X_MUTEX_DEFINED
typedef struct x_mutex_st x_mutex;
#endif

struct x_mutex_st
{
#ifdef X_OS_WIN
	CRITICAL_SECTION *section;
#else
	pthread_mutex_t mutex;
#endif
};

static inline int x_mutex_init(x_mutex *lock)
{
#ifdef X_OS_WIN
	while (!(lock->section = malloc(sizeof *lock->section))) {
		errno = 0;
		Sleep(10);
	}
	InitializeCriticalSection(lock->section);
#else
	if (pthread_mutex_init(&lock->mutex, NULL))
		return -1;
#endif
	return 0;
}

#ifdef X_OS_WIN
static inline int __x_mutex_init_win32(x_mutex *lock)
{
	if (!lock->section)
	{
		CRITICAL_SECTION *secp = NULL;
		while (!(secp = malloc(sizeof *secp))) {
			errno = 0;
			Sleep(10);
		}
		InitializeCriticalSection(secp);
		if (InterlockedCompareExchangePointer((PVOID*)&lock->section, secp, NULL)) {
			DeleteCriticalSection(lock->section);
			free(secp);
		}
	}
	return 0;
}
#endif

static inline int x_mutex_lock(x_mutex *lock)
{

#ifdef X_OS_WIN
	if (__x_mutex_init_win32(lock))
		return -1;
	EnterCriticalSection(lock->section);
#else
	if (pthread_mutex_lock(&lock->mutex))
		return -1;
#endif
	return 0;
}

static inline int x_mutex_trylock( x_mutex *lock)
{
#ifdef X_OS_WIN
	if (__x_mutex_init_win32(lock))
		return -1;
	if (TryEnterCriticalSection(lock->section) == 0) {
		return 1;
	}
#else
	int ret = pthread_mutex_trylock(&lock->mutex);
	if (ret == EBUSY)
		return 1;
	if (ret)
		return -1;
#endif
	return 0;
}

static inline int x_mutex_unlock( x_mutex *lock)
{
#ifdef X_OS_WIN
	if (!lock->section) {
		errno = EPERM;
		return -1;
	}
	LeaveCriticalSection(lock->section);
#else
	if (pthread_mutex_unlock(&lock->mutex))
		return -1;
#endif
	return 0;
}

static inline void x_mutex_destroy( x_mutex *lock)
{
	assert(lock != NULL);
#ifdef X_OS_WIN
	if (!lock->section)
		return;
	DeleteCriticalSection(lock->section);
	free(lock->section);
#else
	(void)pthread_mutex_destroy(&lock->mutex);
#endif
}

#endif
