/*
 * Copyright (c) 2023,2025 Li Xilin <lixilin@gmx.com>
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

#ifndef X_PATH_H
#define X_PATH_H

#include "uchar.h"
#include "detect.h"
#include <limits.h>
#include <stdbool.h>

#ifdef X_OS_WIN
#  include <windef.h>
#  define X_PATH_SEP_CHAR L'\\'
#  define X_PATH_SEP x_u("\\")
#  define X_PATH_MAX MAX_PATH + 1
#  define X_PATHNAME_MAX 260
#else
#  define X_PATH_SEP_CHAR L'/'
#  define X_PATH_SEP x_u("/")
#  define X_PATH_MAX PATH_MAX
#  define X_PATHNAME_MAX NAME_MAX
#endif

void x_path_empty(x_uchar *path);
x_uchar *x_path_fixsep(x_uchar *path);
x_uchar *x_path_trim(x_uchar *path);
bool x_path_is_absolute(const x_uchar *path);
x_uchar *x_path_push(x_uchar *path, size_t size, const x_uchar *name);
const x_uchar *x_path_pop(x_uchar *path);
x_uchar *x_path_join(x_uchar *path, size_t size, ...);
x_uchar *x_path_normalize(x_uchar *path);
x_uchar *x_path_realize(const x_uchar *path, x_uchar *resolved_path, size_t size);
const x_uchar *x_path_basename(const x_uchar *path);
const x_uchar *x_path_extname(const x_uchar *path);
x_uchar *x_path_getcwd(x_uchar *path, size_t size);
int x_path_setcwd(const x_uchar *path);
x_uchar *x_path_homedir(x_uchar *path, size_t size);
x_uchar *x_path_tmpdir(x_uchar *path, size_t size);

#endif

