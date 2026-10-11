/**
 * Copyright (c) 2025-2026 Li Xilin <lixilin@gmx.com>, Shenyang, China
 * Copyright (c) 2021-2023 Eyal Rozenberg <eyalroz1@gmx.com>, Haifa, Palestine/Israel
 * Copyright (c) 2014-2019 Marco Paland <info@paland.com>, PALANDesign Hannover, Germany
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

#include "linkage.h"
#include "types.h"

X_EXTERN_C_BEGIN

int x_printf(const x_uchar* format, ...);
int x_vprintf(const x_uchar* format, va_list arg);
int x_sprintf(x_uchar* s, const x_uchar* format, ...);
int x_vsprintf(x_uchar* s, const x_uchar* format, va_list arg);
int x_bprintf(x_strbuf *sb, const x_uchar* format, ...);
int x_vbprintf(x_strbuf *sb, const x_uchar* format, va_list arg);
int x_snprintf(x_uchar* s, size_t count, const x_uchar* format, ...);
int x_vsnprintf(x_uchar* s, size_t count, const x_uchar* format, va_list arg);
int x_ctprintf(int (*out)(x_uchar c, void* ctx), void* ctx, const x_uchar* format, ...);
int x_vctprintf(int (*out)(x_uchar c, void* ctx), void* ctx, const x_uchar* format, va_list arg);
int x_stprintf(int (*out)(x_uchar *s, size_t len, void* ctx), void* ctx, const x_uchar* format, ...);
int x_vstprintf(int (*out)(x_uchar *s, size_t len, void* ctx), void* ctx, const x_uchar* format, va_list arg);
int x_putchar(x_uchar ch);

X_EXTERN_C_END

#endif

