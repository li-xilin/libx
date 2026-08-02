/*
 * Copyright (c) 2026 Li Xilin <lixilin@gmx.com>
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
#ifndef X_UIDPOOL_H
#define X_UIDPOOL_H

#include <x/types.h>
#include <x/sklist.h>

#define X_BADUID 0

struct x_uidlink_st
{
	x_link link;
	uint32_t num;
};

struct x_uidpool_st {
	x_sklist uid_set;
	uint32_t uid_count;
	uint32_t next_uid;
	uint32_t max_uid;
};

void x_uidpool_init(x_uidpool *pool, uint32_t max_uid);
void x_uidpool_free(x_uidpool *pool);
int x_uidpool_get(x_uidpool *pool, x_uidlink *link);
void x_uidpool_put(x_uidpool *pool, x_uidlink *link);
x_uidlink *x_uidpool_find(x_uidpool *pool, uint32_t uid);

#endif

