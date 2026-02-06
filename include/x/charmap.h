/*
 * Copyright (c) 2026 Li Xilin <lixilin@gmx.com>
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
#ifndef X_CHARMAP_H
#define X_CHARMAP_H

#include "x/types.h"

typedef enum {
	X_CHARMAP_BMP_TO_GBK = 0,
	X_CHARMAP_BMP_TO_BIG5,
	X_CHARMAP_BMP_TO_EUCKR,
	X_CHARMAP_BMP_TO_SHIFTJIS,
	X_CHARMAP_GBK_TO_BMP,
	X_CHARMAP_BIG5_TO_BMP,
	X_CHARMAP_EUCKR_TO_BMP,
	X_CHARMAP_SHIFTJIS_TO_BMP,
	X_CHARMAP_CONV_BITS = 0x0F,

	X_CHARMAP_BMP_BE = 0x10,
	X_CHARMAP_BMP_LE = 0,
	X_CHARMAP_ENDIAN_BITS = 0x10,
} x_charmap_mode;

bool x_charmap_get(uint16_t chr, uint16_t *out, x_charmap_mode mode);
size_t x_charmap_gets(const uint16_t *src, uint16_t *dst, size_t n, uint16_t invalid_char, x_charmap_mode mode);
size_t x_charmap_strlen(const uint16_t *str);

#endif

