/**
 * @author (c) Li Xilin (lixilin@gmx.com)
 *             2025, Shenyang, China
 * @author (c) Eyal Rozenberg <eyalroz1@gmx.com>
 *             2021-2023, Haifa, Palestine/Israel
 * @author (c) Marco Paland (info@paland.com)
 *             2014-2019, PALANDesign Hannover, Germany
 *
 * @note Others have made smaller contributions to this file: see the
 * contributors page at https://github.com/eyalroz/printf/graphs/contributors
 * or ask one of the authors.
 *
 * @brief Small stand-alone implementation of the printf family of functions
 * (`(v)printf`, `(v)s(n)printf` etc., geared towards use on embedded systems
 * with a very limited resources.
 *
 * @note the implementations are thread-safe; re-entrant; use no functions from
 * the standard library; and do not dynamically allocate any memory.
 *
 * @license The MIT License (MIT)
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

#ifndef X_PRINTF_H
#define X_PRINTF_H

#include "macros.h"
#include "uchar.h"
#ifdef __cplusplus
#  include <cstdarg>
#  include <cstddef>
extern "C" {
#else
#  include <stdarg.h>
#  include <stddef.h>
#endif

int x_printf(const x_uchar* format, ...) X_ATTR_PRINTF(1, 2);
int x_vprintf(const x_uchar* format, va_list arg) X_ATTR_VPRINTF(1);
int x_sprintf(x_uchar* s, const x_uchar* format, ...) X_ATTR_PRINTF(2, 3);
int x_vsprintf(x_uchar* s, const x_uchar* format, va_list arg) X_ATTR_VPRINTF(2);
int x_snprintf(x_uchar* s, size_t count, const x_uchar* format, ...) X_ATTR_PRINTF(3, 4);
int x_vsnprintf(x_uchar* s, size_t count, const x_uchar* format, va_list arg) X_ATTR_VPRINTF(3);
int x_fctprintf(int (*out)(x_uchar c, void* ctx), void* ctx, const x_uchar* format, ...) X_ATTR_PRINTF(3, 4);
int x_vfctprintf(int (*out)(x_uchar c, void* ctx), void* ctx, const x_uchar* format, va_list arg) X_ATTR_VPRINTF(3);
int x_putchar(x_uchar ch);

#ifdef __cplusplus
}
#endif

#endif

