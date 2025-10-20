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

#ifndef X_RWLOCK_H
#define X_RWLOCK_H

#include "types.h"
#ifdef X_OS_WIN
#include <synchapi.h>
#define X_RWLOCK_INIT { .bExclusive = FALSE, .SrwLock = SRWLOCK_INIT }
#else
#include <pthread.h>
#define X_RWLOCK_INIT { .rwlock = PTHREAD_RWLOCK_INITIALIZER }
#endif

struct x_rwlock_st
{
#ifdef X_OS_WIN
	SRWLOCK SrwLock;
	BOOLEAN bExclusive;
#else
	pthread_rwlock_t rwlock;
#endif
};

int x_rwlock_init(x_rwlock *lock);
int x_rwlock_rlock(x_rwlock *lock);
int x_rwlock_try_rlock(x_rwlock *lock);
int x_rwlock_wlock(x_rwlock *lock);
int x_rwlock_try_wlock(x_rwlock *lock);
int x_rwlock_unlock(x_rwlock *lock);
void x_rwlock_destroy(x_rwlock *lock);

#endif
