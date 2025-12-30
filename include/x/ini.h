/*
 * Copyright (c) 2023-2024 Li Xilin <lixilin@gmx.com>
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

#ifndef X_INI_H
#define X_INI_H

#include "types.h"
#include "macros.h"
#include "list.h"
#include "flowctl.h"
#include <stdio.h>

#define X_INI_EBADNAME 1
#define X_INI_ETOOLONG 2
#define X_INI_ESYNTAX 3

typedef int x_ini_parse_error_f(int lineno, int error, void *args);

struct x_ini_section_st
{
	x_link link;
	x_list opt_list;
	char *name;
	size_t hash;
	char *comment;
};

struct x_ini_option_st
{
	x_link link;
	size_t hash;
	char *key;
	char *index;
	char *val;
	char *comment;
};

struct x_ini_st
{
	char allowed_ch[16];
	size_t size;
	x_list sec_list;
};

x_ini *x_ini_create(const char *ext_keych);

x_ini *x_ini_load(FILE *fp, const char *ext_keych, x_ini_parse_error_f *error_cb, void *args);

void x_ini_dump(const x_ini * d, FILE * out);

void x_ini_free(x_ini * d);

const char *x_ini_get(const x_ini *d, const char *sec_name, const char *key);

char *x_ini_path_get(const x_ini *d, const char *path_fmt, ...);

char *x_ini_path_vget(const x_ini *d, const char *path_fmt, va_list ap);

int x_ini_path_set(x_ini *d, const char *path, const char *fmt, ...);

int x_ini_set(x_ini *d, const char *sec_name, const char *key, const char *val, const char *comment);

int x_ini_push_sec(x_ini *d, const char *sec_name, const char *comment);

int x_ini_push_opt(x_ini *d, const char *key, const char *val, const char *comment);

void x_ini_unset(x_ini *d, const char *sec_name, const char *key);

bool x_ini_check_section_name(const char *name);

bool x_ini_check_key_name(const char *name, const char *ext_chars);

const char *x_ini_strerror(int errcode);

int x_ini_get_bool(const x_ini *d, const char *path_fmt, int dft_value, ...);

int x_ini_get_int(const x_ini *d, const char *path_fmt, int dft_value, ...);

double x_ini_get_float(const x_ini *d, const char *path_fmt, double dft_value, ...);

char *x_ini_get_str(const x_ini *d, const char *path_fmt, char *dft_value, ...);

void x_ini_pure(x_ini *d);

#define x_ini_foreach_section(sec, d) \
		x_list_foreach(__x_ini_foreach_link, &(d)->sec_list) \
			x_block_var(struct x_ini_section_st *sec = x_container_of( \
						__x_ini_foreach_link, struct x_ini_section_st, link))

#define x_ini_foreach_option(opt, sec) \
		x_list_foreach(__x_ini_foreach_link, &(sec)->opt_list) \
			x_block_var(struct x_ini_option_st *opt = x_container_of( \
						__x_ini_foreach_link, struct x_ini_option_st, link))

#endif

