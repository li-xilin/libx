/*
 * Copyright (c) 2024-2025 Li hsilin <lihsilyn@gmail.com>
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

#include "x/cond.h"
#include "x/errno.h"
#include "x/mutex.h"
#include "x/memory.h"
#include "x/assert.h"
#ifdef X_OS_WIN
#include <synchapi.h>
#include <windef.h>
#include <winbase.h>
#else
#include <pthread.h>
#endif
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

#ifdef X_OS_WIN
static inline void cond_init_competitive(x_cond *cond)
{
	if (!cond->condvar) {
		CONDITION_VARIABLE *condp = x_malloc(NULL, sizeof *condp);
		if (InterlockedCompareExchangePointer((void *volatile *)&cond->condvar, condp, NULL))
			x_free(condp);
	}
}
#endif

int x_cond_init(x_cond *cond)
{
	assert(cond);
#ifdef X_OS_WIN
	cond->condvar = NULL;
	cond_init_competitive(cond);
	InitializeConditionVariable(cond->condvar);
#else
	pthread_condattr_t attr;
	pthread_condattr_init(&attr);
	pthread_condattr_setclock(&attr, CLOCK_MONOTONIC);
	if (pthread_cond_init(&cond->cond, &attr)) {
		pthread_condattr_destroy(&attr);
		return -1;
	}
	pthread_condattr_destroy(&attr);
#endif
	return 0;
}

int x_cond_sleep(x_cond *cond, x_mutex *mutex, int millise)
{
	assert(cond);
	assert(mutex);
#ifdef X_OS_WIN
	cond_init_competitive(cond);
	if (!SleepConditionVariableCS(cond->condvar, mutex->section,
				millise < 0 ? INFINITE : millise)) {
		if (GetLastError() == ERROR_TIMEOUT) {
			errno = ETIMEDOUT;
			return -1;
		}
		fprintf(stderr, "SleepConditionVariableCS failed (%ld)\n", GetLastError());
		abort();
	}
#else
	if (millise < 0)
		return - !!pthread_cond_wait(&cond->cond, &mutex->mutex);

	const uint64_t NS_PER_S = (1000 * 1000 * 1000);
	struct timespec spec;
	if (clock_gettime(CLOCK_MONOTONIC, &spec) == -1)
		return -1;
	uint32_t sec = millise / 1000;
	uint32_t nsec = (millise % 1000) * 1000 * 1000;
	if ((NS_PER_S - spec.tv_nsec) > nsec)
		spec.tv_sec += sec, spec.tv_nsec += nsec;
	else
		spec.tv_sec += sec + 1, spec.tv_nsec = nsec - (NS_PER_S - spec.tv_nsec);
	// Shall not return EINTR
	int ret = pthread_cond_timedwait(&cond->cond, &mutex->mutex, &spec);
	if (!ret)
		return 0;
	else {
		errno = ret;
		return -1;
	}

#endif
	return 0;
}

void x_cond_wake(x_cond *cond)
{
	assert(cond != NULL);
#ifdef X_OS_WIN
	cond_init_competitive(cond);
	WakeConditionVariable(cond->condvar);
#else
	int err = pthread_cond_signal(&cond->cond);
	if (err) {
		fprintf(stderr, "pthread_cond_signal failed (%s)\n", strerror(err));
		abort();
	}
#endif
}

void x_cond_wake_all(x_cond *cond)
{
	assert(cond != NULL);
#ifdef X_OS_WIN
	cond_init_competitive(cond);
	WakeAllConditionVariable(cond->condvar);
#else
	int err = pthread_cond_broadcast(&cond->cond);
	if (err) {
		fprintf(stderr, "pthread_cond_broadcast failed (%s)\n", strerror(err));
		abort();
	}
#endif
}

void x_cond_destroy(x_cond *cond)
{
	assert(cond != NULL);
#ifdef X_OS_WIN
	if (!cond->condvar)
		return;
	x_free(cond->condvar);
#else
	(void)pthread_cond_destroy(&cond->cond);
#endif
}

