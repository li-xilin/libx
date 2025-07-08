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

#ifndef X_SYS_H
#define X_SYS_H

#include "uchar.h"
#include <stddef.h>
#include <stdlib.h>
#include <stdbool.h>
#include <time.h>

int x_sys_mkdir(const x_uchar *path, int mode);
int x_sys_unlink(const x_uchar *path);
int x_sys_rename(const x_uchar *path, const x_uchar *new_path);
int x_sys_copy(const x_uchar *path, const x_uchar *new_path);
int x_sys_link(const x_uchar *path, const x_uchar *link_path);
/* 'dir_link' parameter specifies if the symbolic link is for a directory in Windows. */
int x_sys_symlink(const x_uchar *path, const x_uchar *link_path, bool dir_link);
const x_uchar *x_sys_getenv(const x_uchar *name);
int x_sys_setenv(const x_uchar *name, const x_uchar *value);
int x_sys_utime(const x_uchar *path, time_t atime, time_t mtime);
int x_sys_nprocs(void);

#endif

