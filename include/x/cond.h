/*
 * Copyright (c) 2024 Li hsilin <lihsilyn@gmail.com>
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

#ifndef X_COND_H
#define X_COND_H

#include "x/errno.h"
#include "x/types.h"
#include "x/mutex.h"
#include <assert.h>
#include <time.h>
#include <stdlib.h>

#ifdef X_OS_WIN
#include <synchapi.h>
#define X_COND_INIT { .condvar = NULL }
#else
#include <pthread.h>
#define X_COND_INIT { .cond = PTHREAD_COND_INITIALIZER }
#endif

struct x_cond_st
{
#ifdef X_OS_WIN
	CONDITION_VARIABLE *condvar;
#else
	pthread_cond_t cond;
#endif
};

#ifdef X_OS_WIN
static inline int __x_cond_init_competitive(x_cond *cond)
{
	if (!cond->condvar) {
		CONDITION_VARIABLE *condp = NULL;
		while (!(condp = malloc(sizeof *condp))) {
			errno = 0;
			Sleep(20);
		}
		if (InterlockedCompareExchangePointer((void *volatile *)&cond->condvar, condp, NULL))
			free(condp);
	}
	return 0;
}
#endif

static inline int x_cond_init(x_cond *cond)
{
	assert(cond);
#ifdef X_OS_WIN
	while (!(cond->condvar = malloc(sizeof *cond->condvar))) {
		errno = 0;
		Sleep(20);
	}
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

static inline int x_cond_sleep(x_cond *cond, x_mutex *mutex, int millise)
{
	assert(cond);
	assert(mutex);
#ifdef X_OS_WIN
	if (!mutex->section) {
		errno = EPERM;
		return -1;
	}
	if (__x_cond_init_competitive(cond))
		return -1;

	if (!SleepConditionVariableCS(cond->condvar, mutex->section,
				millise < 0 ? INFINITE : millise)) {
		x_eval_errno();
		return -1;
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

static inline int x_cond_wake(x_cond *cond)
{
	assert(cond);
#ifdef X_OS_WIN
	if (__x_cond_init_competitive(cond)) {
		x_eval_errno();
		return -1;
	}
	WakeConditionVariable(cond->condvar);
#else
	if (pthread_cond_signal(&cond->cond)) {
		return -1;
	}
#endif
	return 0;
}

static inline int x_cond_wake_all(x_cond *cond)
{
	assert(cond);
#ifdef X_OS_WIN
	if (__x_cond_init_competitive(cond)) {
		x_eval_errno();
		return -1;
	}
	WakeAllConditionVariable(cond->condvar);
#else
	if (pthread_cond_broadcast(&cond->cond)) {
		return -1;
	}
#endif
	return 0;
}

static inline void x_cond_destroy(x_cond *cond)
{
	assert(cond);
#ifdef X_OS_WIN
	if (!cond->condvar)
		return;
	free(cond->condvar);
#else
	(void)pthread_cond_destroy(&cond->cond);
#endif
}

#endif

