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

#include "types.h"
#include "detect.h"

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

int x_cond_init(x_cond *cond);

int x_cond_sleep(x_cond *cond, x_mutex *mutex, int millise);

void x_cond_wake(x_cond *cond);

void x_cond_wake_all(x_cond *cond);

void x_cond_destroy(x_cond *cond);

#endif

