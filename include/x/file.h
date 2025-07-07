/*
 * Copyright (c) 2023 Li Xilin <lixilin@gmx.com>
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

#ifndef X_FILE_H
#define X_FILE_H

#include "uchar.h"
#include <stdio.h>
#include <stdarg.h>

FILE* x_fdopen(intptr_t handle, const x_uchar *mode);
FILE *x_fopen(const x_uchar *path, const x_uchar *mode);
FILE *x_freopen(const x_uchar *path, const x_uchar *mode, FILE *stream);

int x_setinput(FILE *stream);
int x_setoutput(FILE *stream);
int x_isatty(FILE *file);

int x_vfprintf(FILE *stream, const x_uchar *format, va_list ap);
int x_fprintf(FILE *stream, const x_uchar *format, ...);
int x_vprintf(const x_uchar *format, va_list ap);
int x_printf(const x_uchar *format, ...);
int x_fputs(const x_uchar *ws, FILE *stream);
int x_fputc(x_uchar c, FILE *stream);
int x_putchar(x_uchar c);

int x_vfscanf(FILE *stream, const x_uchar *format, va_list ap);
int x_fscanf(FILE *stream, const x_uchar *format, ...);
int x_vscanf(const x_uchar *format, va_list ap);
int x_scanf(const x_uchar *format, ...);
x_uchar *x_fgets(x_uchar *ws, int n, FILE *stream);
int x_fgetc(FILE *stream);
int x_getchar(void);

#endif

