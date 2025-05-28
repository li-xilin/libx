/*
 * Copyright (c) 2022-2024 Li Xilin <lixilin@gmx.com>
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

#ifndef X_FLOWCTL_H
#define X_FLOWCTL_H

#include "macros.h"
#include <stddef.h>
#include <stdint.h>
#include <setjmp.h>

#define x_block for(register int __ax_block_flag = 0; __ax_block_flag != 1; __ax_block_flag = 1)

#define x_repeat(_n) for(size_t _ = 0; _ != (_n); _++)

#define x_forever for(;;)

#define x_forvalues(_t, ...) \
	for (intptr_t __x_i = 1; __x_i <= X_NARG_T(_t, __VA_ARGS__); __x_i = - __x_i, __x_i++) \
		for (_t _ = ((_t[]){ __VA_ARGS__ })[__x_i - 1]; __x_i > 0; __x_i = - __x_i)

#define __X_FORRANGE_STEP(...) \
	(X_NARG_T(ptrdiff_t, __VA_ARGS__) > 1 ? x_p(ptrdiff_t, __VA_ARGS__)[1] : 1)

#define __X_FORRANGE_LAST(...) \
	x_p(ptrdiff_t, __VA_ARGS__)[0]

#define __X_FORRANGE_END(first, ...) \
	(__X_FORRANGE_LAST(__VA_ARGS__) \
	 - (__X_FORRANGE_LAST(__VA_ARGS__) - first + (__X_FORRANGE_STEP(__VA_ARGS__) > 0 ? -1 : 1)) % __X_FORRANGE_STEP(__VA_ARGS__) \
	 + __X_FORRANGE_STEP(__VA_ARGS__)) + (__X_FORRANGE_STEP(__VA_ARGS__) > 0 ? -1 : 1)

#define x_forrange(first, ...) \
	for (ptrdiff_t _ = first, __x_forrange_end = __X_FORRANGE_END(first, __VA_ARGS__); \
		_ != __x_forrange_end; \
		_ += __X_FORRANGE_STEP(__VA_ARGS__))

#define x_routine(name) jmp_buf __x_jmpbuf_##name; while (0) __x_label_##name: \
                for (int __x_tmp = 0;  ; __x_tmp = 1) if (__x_tmp) longjmp(__x_jmpbuf_##name, 1); else \
                for (; !__x_tmp ; __x_tmp = 1)

#define x_routine_call(name) if (!setjmp(__x_jmpbuf_##name)) goto __x_label_##name;

#endif
