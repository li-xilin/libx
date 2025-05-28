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
#include <stdio.h>

#define X_INI_EBADNAME 1
#define X_INI_ETOOLONG 2
#define X_INI_ESYNTAX 3

typedef int x_ini_parse_error_f(unsigned lineno, unsigned error, void *args);

x_ini *x_ini_create(void);

x_ini *x_ini_load(FILE *fp, x_ini_parse_error_f *error_cb, void *args);

void x_ini_dump(const x_ini * d, FILE * out);

void x_ini_free(x_ini * d);

const char *x_ini_get(const x_ini *d, const char *sec_name, const char *key);

char *x_ini_path_get(const x_ini *d, const char *path);

int x_ini_path_set(x_ini *d, const char *path, const char *fmt, ...);

int x_ini_set(x_ini *d, const char *sec_name, const char *key, const char *val, const char *comment);

int x_ini_push_sec(x_ini *d, const char *sec_name, const char *comment);

int x_ini_push_opt(x_ini *d, const char *key, const char *val, const char *comment);

void x_ini_unset(x_ini *d, const char *sec_name, const char *key);

bool x_ini_check_section_name(const char *name);

bool x_ini_check_key_name(const char *name, const char *ext_chars);

const char *x_ini_strerror(int errcode);

int x_ini_get_bool(const x_ini *d, const char *path, bool dft_value);

int x_ini_get_int(const x_ini *d, const char *path, int dft_value);

char *x_ini_get_str(const x_ini *d, const char *path, char *dft_value);

void x_ini_pure(x_ini *d);

#endif

