/*
 * Copyright (c) 2023-2025 Li Xilin <lixilin@gmx.com>
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

#include "x/rwlock.h"
#include <assert.h>
#include <errno.h>

int x_rwlock_init(x_rwlock *lock)
{
	assert(lock);
#ifdef X_OS_WIN
	InitializeSRWLock(&lock->SrwLock);
	lock->bExclusive = FALSE;
#else
	if (pthread_rwlock_init(&lock->rwlock, NULL)) {
		return -1;
	}
#endif
	return 0;
}

int x_rwlock_rlock(x_rwlock *lock)
{
	assert(lock);
#ifdef X_OS_WIN
	AcquireSRWLockShared(&lock->SrwLock);
#else
	if (pthread_rwlock_rdlock(&lock->rwlock)) {
		return -1;
	}
#endif
	return 0;
}

int x_rwlock_try_rlock(x_rwlock *lock)
{
	assert(lock);
#ifdef X_OS_WIN
	if (!TryAcquireSRWLockShared(&lock->SrwLock)) {
		return -1;
	}
#else
	if (pthread_rwlock_tryrdlock(&lock->rwlock)) {
		return -1;
	}
#endif
	return 0;
}

int x_rwlock_wlock(x_rwlock *lock)
{
	assert(lock);
#ifdef X_OS_WIN
	AcquireSRWLockExclusive(&lock->SrwLock);
	lock->bExclusive = TRUE;
#else

	if (pthread_rwlock_wrlock(&lock->rwlock)) {
		return -1;
	}
#endif
	return 0;
}

int x_rwlock_try_wlock(x_rwlock *lock)
{
	assert(lock);
#ifdef X_OS_WIN
	if (!TryAcquireSRWLockExclusive(&lock->SrwLock))
		return 1;
	lock->bExclusive = TRUE;
#else
	int ret = pthread_rwlock_trywrlock(&lock->rwlock);
	if (ret == EBUSY)
		return 1;
	if (ret) {
		return -1;
	}
#endif
	return 0;
}

int x_rwlock_unlock(x_rwlock *lock)
{
	assert(lock);
#ifdef X_OS_WIN
	if (lock->bExclusive) {
		lock->bExclusive = FALSE;
		ReleaseSRWLockExclusive(&lock->SrwLock);
	} else
		ReleaseSRWLockShared(&lock->SrwLock);
#else
	if (pthread_rwlock_unlock(&lock->rwlock)) {
		return -1;
	}
#endif
	return 0;
}

void x_rwlock_destroy(x_rwlock *lock)
{
	assert(lock);
#ifndef X_OS_WIN
	(void)pthread_rwlock_destroy(&lock->rwlock);
#endif
}

