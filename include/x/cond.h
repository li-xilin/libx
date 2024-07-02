#ifndef X_COND_H
#define X_COND_H

#include "mutex.h"
#include "errno.h"
#include <stdint.h>
#include <assert.h>
#include <time.h>
#include <errno.h>
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

#ifndef X_COND_DEFINED
#define X_COND_DEFINED
typedef struct x_cond_st x_cond;
#endif

#ifdef X_OS_WIN
static inline int __x_cond_init_competitive(x_cond *cond)
{
	if (!cond->condvar) {
		CONDITION_VARIABLE *condp = NULL;
		while (!(condp = malloc(sizeof *condp))) {
			errno = 0;
			Sleep(20);
		}
		if (InterlockedCompareExchangePointer(&cond->condvar, condp, NULL))
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
	if (pthread_cond_init(&cond->cond, NULL))
		return -1;
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
	if (__x_cond_init_competitive(cond)) {
		x_error_occur();
		return -1;
	}
	
	if (!SleepConditionVariableCS(cond->condvar, mutex->section,
				millise < 0 ? INFINITE : millise)) {
		x_error_occur();
		if (GetLastError() == ERROR_TIMEOUT)
			return 1;
		return -1;
	}
#else
	if (millise < 0)
		return - !!pthread_cond_wait(&cond->cond, &mutex->mutex);

        const int NS_PER_MS = (1000 * 1000 * 1000);
        struct timespec spec;
        if (clock_gettime(CLOCK_REALTIME, &spec) == -1)
                return -1;
        uint32_t sec = millise / 1000;
        uint32_t nsec = (millise % 1000) * 1000 * 1000;
        if ((NS_PER_MS - spec.tv_nsec) < nsec)
                spec.tv_sec += sec, spec.tv_nsec += nsec;
        else
                spec.tv_sec += sec + 1, spec.tv_nsec += (spec.tv_nsec + nsec) % NS_PER_MS;

	// Shall not return EINTR
        int ret = pthread_cond_timedwait(&cond->cond, &mutex->mutex, &spec);
        if (ret == ETIMEDOUT)
                return 1;
        if (!ret)
                return 0;
        else
                return -1;

#endif
	return 0;
}

static inline int x_cond_wake(x_cond *cond)
{
	assert(cond);
#ifdef X_OS_WIN
	if (__x_cond_init_competitive(cond)) {
		x_error_occur();
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
		x_error_occur();
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

