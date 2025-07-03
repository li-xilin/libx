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

#include "types.h"
#include "detect.h"

#ifdef X_OS_WIN
#include <synchapi.h>
#define X_MUTEX_INIT { .section = NULL }
#else
#include <pthread.h>
#define X_MUTEX_INIT { .mutex = PTHREAD_MUTEX_INITIALIZER }
#endif

struct x_mutex_st
{
#ifdef X_OS_WIN
	CRITICAL_SECTION *section;
#else
	pthread_mutex_t mutex;
#endif
};

int x_mutex_init(x_mutex *lock);

void x_mutex_lock(x_mutex *lock);

bool x_mutex_trylock(x_mutex *lock);

void x_mutex_unlock(x_mutex *lock);

void x_mutex_destroy(x_mutex *lock);

#endif
