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

#include "x/mutex.h"
#include "x/types.h"
#include "x/detect.h"
#include "x/assert.h"
#include "x/memory.h"
#ifdef X_OS_WIN
#include <synchapi.h>
#else
#include <pthread.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

void x_mutex_init(x_mutex *lock)
{
#ifdef X_OS_WIN
	lock->section = x_malloc(NULL, sizeof *lock->section);
	InitializeCriticalSection(lock->section);
#else
	(void)pthread_mutex_init(&lock->mutex, NULL);
#endif
}

#ifdef X_OS_WIN
static inline void mutex_init_win32(x_mutex *lock)
{
	assert(lock != NULL);
	if (!lock->section) {
		CRITICAL_SECTION *secp = x_malloc(NULL, sizeof *secp);
		InitializeCriticalSection(secp);
		if (InterlockedCompareExchangePointer((PVOID*)&lock->section, secp, NULL)) {
			DeleteCriticalSection(lock->section);
			x_free(secp);
		}
	}
}
#endif

void x_mutex_lock(x_mutex *lock)
{
	assert(lock != NULL);
#ifdef X_OS_WIN
	mutex_init_win32(lock);
	EnterCriticalSection(lock->section);
#else
	int err = pthread_mutex_lock(&lock->mutex);
	if (err) {
		fprintf(stderr, "fatal: pthread_mutex_lock failed (%s)", strerror(err));
		abort();
	}
#endif
}

bool x_mutex_trylock(x_mutex *lock)
{
	assert(lock != NULL);
#ifdef X_OS_WIN
	mutex_init_win32(lock);
	return TryEnterCriticalSection(lock->section);
#else
	int err = pthread_mutex_trylock(&lock->mutex);
	switch (err) {
		case 0:
			return true;
		case EBUSY:
			return false;
		default:
			fprintf(stderr, "fatal: pthread_mutex_trylock failed (%s)", strerror(err));
			abort();
			return false;
	}
#endif
}

void x_mutex_unlock(x_mutex *lock)
{
	assert(lock != NULL);
#ifdef X_OS_WIN
	if (!lock->section) {
		fprintf(stderr, "fatal: x_mutex object not initialized");
		abort();
	}
	LeaveCriticalSection(lock->section);
#else
	int err = pthread_mutex_unlock(&lock->mutex);
	if (err) {
		fprintf(stderr, "fatal: pthread_mutex_unlock failed (%s)", strerror(err));
		abort();
	}
#endif
}

void x_mutex_destroy(x_mutex *lock)
{
	assert(lock != NULL);
#ifdef X_OS_WIN
	assert(!lock->section);
	DeleteCriticalSection(lock->section);
	x_free(lock->section);
#else
	(void)pthread_mutex_destroy(&lock->mutex);
#endif
}

