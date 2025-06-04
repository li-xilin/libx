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

#ifndef X_DUMP_H
#define X_DUMP_H

#include "macros.h"
#include "types.h"
#include <stdio.h>

typedef int x_dump_out_cb_f(const char *str, size_t len, void *ctx);

struct x_dump_format_st
{
	int (*snumber)(intmax_t value, x_dump_out_cb_f *out_cb, void *ctx);
	int (*unumber)(uintmax_t value, x_dump_out_cb_f *out_cb, void *ctx);
	int (*fnumber)(double value, x_dump_out_cb_f *out_cb, void *ctx);
	int (*pointer)(const void *value, x_dump_out_cb_f *out_cb, void *ctx);
	int (*string)(const char *value, size_t length, x_dump_out_cb_f *out_cb, void *ctx);
	int (*wstring)(const wchar_t *value, size_t length, x_dump_out_cb_f *out_cb, void *ctx);
	int (*memory)(const uint8_t *value, size_t size, x_dump_out_cb_f *out_cb, void *ctx);
	int (*symbol)(const char *name, x_dump_out_cb_f *out_cb, void *ctx);
	int (*pair_left)(x_dump_out_cb_f *out_cb, void *ctx);
	int (*pair_midst)(x_dump_out_cb_f *out_cb, void *ctx);
	int (*pair_right)(x_dump_out_cb_f *out_cb, void *ctx);
	int (*block_left)(const char *label, x_dump_out_cb_f *out_cb, void *ctx);
	int (*block_midst)(size_t index, x_dump_out_cb_f *out_cb, void *ctx);
	int (*block_right)(const char *label, x_dump_out_cb_f *out_cb, void *ctx);
	int (*nomem)(x_dump_out_cb_f *out_cb, void *ctx);
	int (*indent)(int depth, x_dump_out_cb_f *out_cb, void *ctx);
};

x_dump *x_dump_int(intmax_t val);
x_dump *x_dump_uint(uintmax_t val);
x_dump *x_dump_float(double val);
x_dump *x_dump_ptr(const void *val);
x_dump *x_dump_str(const char *val);
x_dump *x_dump_wcs(const wchar_t *val);
x_dump *x_dump_mem(const void *ptr, size_t size);
x_dump *x_dump_symbol(const char *sym);
x_dump *x_dump_pair(x_dump *d1, x_dump *d2);
x_dump *x_dump_empty_block(const char* sym, size_t elem_cnt);
x_dump *x_dump_block(const char *sym, ...);
int x_dump_set_name(x_dump *dmp, const char *sym);
void x_dump_bind(x_dump *dmp, int index, x_dump* binding);
void x_dump_free(x_dump *dmp);
int x_dump_fput(const x_dump *dmp, const x_dump_format *format, FILE *fp);
int x_dump_serialize(const x_dump *dmp, const x_dump_format *format, x_dump_out_cb_f *cb, void *ctx);
const x_dump_format *x_dump_default_format(void);
const x_dump_format *x_dump_pretty_format(void);

inline static void x_dump_named_bind(x_dump *dmp, int index, const char *name, x_dump* binding)
{
	x_dump_bind(dmp, index, x_dump_pair(x_dump_symbol(name), binding));
}

#endif
