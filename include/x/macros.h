/*
 * Copyright (c) 2021-2025 Li Xilin <lixilin@gmx.com>
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

#ifndef X_MACROS_H
#define X_MACROS_H

#include "trick.h"
#include "detect.h"
#include <stdint.h>
#include <stddef.h>

#define x_min(a, b) ((a) < (b) ? (a) : (b))
#define x_max(a, b) ((a) > (b) ? (a) : (b))
#define x_cyclic_inc(i, n) ((i + 1) % n)
#define x_cyclic_dec(i, n) ((i + n - 1) % n)
#define x_bit(i) (1LL << i)
#define x_singlebit(n)  ((n) != 0 && !((n) & ((n)-1)))

#define x_unused(_var) ((void)&(_var))

#define x_align(_size, _align) \
	(((_size) + (_align) - 1) / (_align) * (_align))

#define __x_stringy(_x) #_x
#define x_stringy(_x) __x_stringy(_x)

#define x_p(_type, ...) ((_type []) { __VA_ARGS__ })
#define x_pstruct(_type, ...) x_p(_type, { __VA_ARGS__ })

#define x_arrlen(_a) (((sizeof (_a) / sizeof *(_a))) / !((uintptr_t)_a - (uintptr_t)&_a))

#define x_container_of(ptr, type, member) (((type*)((char*)ptr - (offsetof(type, member)))))

#define __X_SIZEOF(i, type) + sizeof(type)
#define x_sizeof(...) (X_TRANSFORM(__X_SIZEOF, __VA_ARGS__))

#if !defined(X_OS_WIN) && defined(X_CC_CLANG)
#  define X_ATTR_PRINTF(one_based_format_index, first_arg) __attribute__((format(printf, (one_based_format_index), (first_arg))))
#  define X_ATTR_VPRINTF(one_based_format_index) X_ATTR_PRINTF((one_based_format_index), 0)
#elif !defined(X_OS_WIN) && defined(X_CC_GNU)
#  if ((__GNUC__ == 4 && __GNUC_MINOR__>= 4) || __GNUC__ > 4)
#    define X_ATTR_PRINTF(one_based_format_index, first_arg) __attribute__((format(gnu_printf, (one_based_format_index), (first_arg))))
#  else
#    define X_ATTR_PRINTF(one_based_format_index, first_arg) __attribute__((format(printf, (one_based_format_index), (first_arg))))
#  endif
#  define X_ATTR_VPRINTF(one_based_format_index) X_ATTR_PRINTF((one_based_format_index), 0)
#else
#  define X_ATTR_PRINTF(one_based_format_index, first_arg)
#  define X_ATTR_VPRINTF(one_based_format_index)
#endif

#endif

