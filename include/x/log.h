/*
 * Copyright (c) 2020-2024 Li Xilin <lixilin@gmx.com>
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

#ifndef X_LOG_H
#define X_LOG_H

#include "assert.h"
#include "uchar.h"
#include <stdarg.h>

#define X_LL_FATAL    0
#define X_LL_ERROR    1
#define X_LL_WARN     2
#define X_LL_INFO     3
#define X_LL_DEBUG    4
#define X_LL_TRACE    5

#define X_LM_NOTRACE (1 << 0)
#define X_LM_NODEBUG (1 << 1)
#define X_LM_NOINFO  (1 << 2)
#define X_LM_NOWARN  (1 << 3)
#define X_LM_NOERROR (1 << 4)
#define X_LM_NOFATAL (1 << 5)
#define X_LM_NOLOC   (1 << 6)
#define X_LM_NOTIME  (1 << 7)

#define X_LM_NOLOG \
	( X_LM_NODEBUG \
	| X_LM_NOINFO \
	| X_LM_NOWARN \
	| X_LM_NOERROR \
	| X_LM_NOFATAL)

#define X_LM_NORMAL \
	( X_LM_NOTRACE \
	| X_LM_NODEBUG)

#define X_LM_ALLLOG 0

#define X_LOG_MX 8192

typedef int x_log_handler_fn(const x_location *loc, void *arg, int level, const x_uchar *text);
int __x_log_print(const x_location *loc, int level, const x_uchar* fmt, ...);
int __x_log_vprint(const x_location *loc, int level, const x_uchar* fmt, va_list ap);
void x_log_set_mode(int mode);
int x_log_mode(void);
x_log_handler_fn x_log_handler_native;
x_log_handler_fn x_log_handler_utf8;
void x_log_set_handler(x_log_handler_fn *f, void *arg);

#define x_log(level, ...) __x_log_print(X_WHERE, level, x_u("") __VA_ARGS__)
#define x_ptrace(...) x_log(X_LL_TRACE, __VA_ARGS__)
#define x_pdebug(...) x_log(X_LL_DEBUG, __VA_ARGS__)
#define x_pinfo(...) x_log(X_LL_INFO, __VA_ARGS__)
#define x_pwarn(...) x_log(X_LL_WARN, __VA_ARGS__)
#define x_perror(...) x_log(X_LL_ERROR, __VA_ARGS__)
#define x_pfatal(...) x_log(X_LL_FATAL, __VA_ARGS__)

#endif

