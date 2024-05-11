/*
 * Copyright (c) 2020-2021, 2023-2024 Li Xilin <lixilin@gmx.com>
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

#ifndef X_ASSERT_H
#define X_ASSERT_H

#include "trick.h"
#include "narg.h"
#include "detect.h"

#include <assert.h>

typedef struct {
        const char *file;
        const char *func;
        int line;
} x_location;

#define X_WHERE ((const x_location[1]) { { __FILE__, __func__, __LINE__ } })

int __x_assert_fail(const x_location *loc, const char* brief, const char* fmt, ...);

#ifndef NDEBUG
# define x_assert(_exp, ...) ((_exp) ? (void)0 : \
		(void)__x_assert_fail(X_WHERE, "assertion failed", __VA_ARGS__))
#else
# define x_assert(_exp, ...) ((void)0)
#endif

#ifdef X_CC_GNU
#define __X_ASSERT_UNUSED __attribute__((unused))
#endif

#ifndef NDEBUG
# define x_static_assert(_exp) typedef char X_CATENATE(__x_static_assert_, \
		__LINE__)[(_exp) ? 1 : -1] __X_ASSERT_UNUSED
#else
# define x_static_assert(_exp) typedef char X_CATENATE(__x_static_assert_, __LINE__)[1] __X_ASSERT_UNUSED
#endif

#define x_assert_not_null(x) x_assert(*&(x), "unexpected NULL value for `%s`", #x);

#endif

