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
#include "x/charmap.h"
extern const uint16_t x_bmp_to_big5[];
extern const uint16_t x_big5_to_bmp[];
extern const uint16_t x_bmp_to_euckr[];
extern const uint16_t x_euckr_to_bmp[];
extern const uint16_t x_bmp_to_gbk[];
extern const uint16_t x_gbk_to_bmp[];
extern const uint16_t x_bmp_to_sjis[];
extern const uint16_t x_sjis_to_bmp[];
extern size_t x_bmp_to_big5_count;
extern size_t x_big5_to_bmp_count;
extern size_t x_bmp_to_euckr_count;
extern size_t x_euckr_to_bmp_count;
extern size_t x_bmp_to_gbk_count;
extern size_t x_gbk_to_bmp_count;
extern size_t x_bmp_to_sjis_count;
extern size_t x_sjis_to_bmp_count;

bool x_charmap_get(uint16_t chr, uint16_t *out, x_charmap_mode mode)
{
	int okey = false;
	const uint16_t *table;
	int i, n, li, hi = 0;
	
	switch (mode & X_CHARMAP_CONV_BITS) {
		case X_CHARMAP_BMP_TO_GBK:
			table = x_bmp_to_gbk;
			hi = x_bmp_to_gbk_count;
			break;
		case X_CHARMAP_BMP_TO_BIG5:
			table = x_bmp_to_big5;
			hi = x_bmp_to_big5_count;
			break;
		case X_CHARMAP_BMP_TO_EUCKR:
			table = x_bmp_to_euckr;
			hi = x_bmp_to_euckr_count;
			break;
		case X_CHARMAP_BMP_TO_SHIFTJIS:
			table = x_bmp_to_sjis;
			hi = x_bmp_to_sjis_count;
			break;
		case X_CHARMAP_GBK_TO_BMP:
			table = x_gbk_to_bmp;
			hi = x_gbk_to_bmp_count;
			goto endian;
		case X_CHARMAP_BIG5_TO_BMP:
			table = x_big5_to_bmp;
			hi = x_big5_to_bmp_count;
			goto endian;
		case X_CHARMAP_EUCKR_TO_BMP:
			table = x_euckr_to_bmp;
			hi = x_euckr_to_bmp_count;
			goto endian;
		case X_CHARMAP_SHIFTJIS_TO_BMP:
			table = x_sjis_to_bmp;
			hi = x_sjis_to_bmp_count;
endian:
			if ((mode & X_CHARMAP_ENDIAN_BITS) == X_CHARMAP_BMP_LE)
				chr = (chr << 8) | (chr >> 8);
			break;
		default:
			goto out;
	}
	if (chr < 0x80) {
		*out = chr;
		okey = true;
		goto out;
	}
	li = 0;
	for (n = 16; n; n--) {
		i = li + (hi - li) / sizeof(uint16_t);
		if (chr == table[i * sizeof(uint16_t)])
			break;
		if (chr > table[i * sizeof(uint16_t)])
			li = i;
		else
			hi = i;
	}
	if (!n) 
		goto out;
	*out = table[i * sizeof(uint16_t) + 1];
	okey = true;
out:
	if (okey) {
		switch (mode & X_CHARMAP_CONV_BITS) {
			case X_CHARMAP_BMP_TO_GBK:
			case X_CHARMAP_BMP_TO_BIG5:
			case X_CHARMAP_BMP_TO_EUCKR:
			case X_CHARMAP_BMP_TO_SHIFTJIS:
				if ((mode & X_CHARMAP_ENDIAN_BITS) == X_CHARMAP_BMP_LE)
					*out = (*out << 8) | (*out >> 8);
				break;
		}
	}
	return okey;
}

size_t x_charmap_gets(const uint16_t *src, uint16_t *dst, size_t dst_max, uint16_t invalid_char, x_charmap_mode mode)
{
	size_t i;
	for (i = 0; i < dst_max && src[i]; i++) {
		if (x_charmap_get(src[i], dst + i, mode))
			continue;
		if (invalid_char == 0) {
			dst[i] = 0;
			return i + 1;
		}
		dst[i] = invalid_char;
	}
	if (i == dst_max)
		return dst_max;
	dst[i] = 0;
	return i + 1;
}

size_t x_charmap_strlen(const uint16_t *str)
{
	size_t len = 0;
	while (str[len])
		len++;
	return len;
}

