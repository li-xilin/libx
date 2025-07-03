/*
 * Copyright (c) 2021-2025 Li Xilin <lixilin@gmx.com>
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

#ifndef X_TSS_H
#define X_TSS_H

#include "detect.h"
#include <errno.h>
#include <assert.h>
#include <stdint.h>

#ifndef X_TSS_DEFINED
#define X_TSS_DEFINED
typedef struct x_tss_st x_tss;
#endif

struct x_tss_st
{
	uint32_t key;
};

typedef void (x_tss_free_f)(void *ptr);

int x_tss_init(x_tss *key, x_tss_free_f *free_cb);

void x_tss_remove(x_tss *key);

void *x_tss_get(x_tss *key);

int x_tss_set(x_tss *key, void *value);


#endif

