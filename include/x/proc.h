/*
 * Copyright (c) 2023,2025 Li Xilin <lixilin@gmx.com>
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

#ifndef X_PROC_H
#define X_PROC_H

#include "uchar.h"
#include "types.h"
#include "file.h"
#include <stdint.h>

#ifndef X_PROC_DEFINED
#define X_PROC_DEFINED
typedef struct x_proc_st x_proc;
#endif

x_proc *x_proc_open(const x_uchar* file, x_uchar *const argv[]);

FILE *x_proc_stdio(const x_proc *proc, int fd);

int x_proc_fclose(x_proc *proc, int fd);

x_pid x_proc_pid(const x_proc *proc);

int x_proc_kill(const x_proc *proc);

int x_proc_close(x_proc *proc);

#endif
