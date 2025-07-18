/*
 * Copyright (c) 2021-2024 Li Xilin <lixilin@gmx.com>
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

#ifndef X_THREAD_H
#define X_THREAD_H

#include "types.h"
#include "assert.h"
#include "detect.h"
#include <errno.h>
#include <assert.h>
#include <stdlib.h>
#include <stdint.h>

typedef int (x_thread_fn)(void);
typedef void (x_thread_clean_fn)(void);

void x_thread_exit(int ret_code);

x_thread *x_thread_create(x_thread_fn *entry, x_thread_clean_fn *cleanup, void *arg);

int x_thread_sleep(unsigned int millise);

int x_thread_join(x_thread *t, int *retcode);

int x_thread_free(x_thread *t);

void x_thread_yield(void);

void x_thread_kill(x_thread *t);

void x_thread_testcancel(int retcode);

void x_thread_cancel(x_thread *t);

void *x_thread_data(void);

uint32_t x_thread_native_id(void);

void x_thread_cleanup(void);

#endif

