/*
 * Copyright (c) 2025 Li Xilin <lixilin@gmx.com>
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
#ifndef X_FWALKER_H
#define X_FWALKER_H

#include "x/types.h"
#include "x/path.h"
#include "x/list.h"
#include "x/memory.h"

struct x_fwalker_st
{
	x_mset mset;
	x_list dir_list;
	x_uchar path[X_PATH_MAX];
};

int x_fwalker_open(x_fwalker *fwalker, const x_uchar *root);
void x_fwalker_close(x_fwalker *fwalker);
x_uchar *x_fwalker_read(x_fwalker *fwalker, x_stat *statbuf);
int x_fwalker_rewind(x_fwalker *fwalker);
void x_fwalker_leave(x_fwalker *fwalker);

#endif

