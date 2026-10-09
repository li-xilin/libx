/*
 * Copyright (c) 2024-2026 Li Xilin <lixilin@gmx.com>
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

#include "x/bitmap.h"
#include "x/flowctl.h"

#include <stdio.h>
#include <stdlib.h>

static uint8_t s_one_count_lut[] = {
	0, 1, 1, 2, 1, 2, 2, 3, 1, 2, 2, 3, 2, 3, 3, 4,
	1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5,
	1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5,
	2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
	1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5,
	2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
	2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
	3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7,
	1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5,
	2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
	2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
	3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7,
	2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
	3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7,
	3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7,
	4, 5, 5, 6, 5, 6, 6, 7, 5, 6, 6, 7, 6, 7, 7, 8,
};

static char s_one_lut[] = {
	-1, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
	4, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
	5, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
	4, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
	6, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
	4, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
	5, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
	4, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
	7, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
	4, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
	5, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
	4, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
	6, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
	4, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
	5, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
	4, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
};

static char s_zero_lut[] = {
	0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0, 4,
	0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0, 5,
	0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0, 4,
	0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0, 6,
	0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0, 4,
	0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0, 5,
	0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0, 4,
	0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0, 7,
	0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0, 4,
	0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0, 5,
	0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0, 4,
	0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0, 6,
	0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0, 4,
	0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0, 5,
	0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0, 4,
	0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0, -1,	
};

static size_t bitmap_find0(x_bitmap *bm, size_t start, size_t len)
{
	if (len == 0)
		return -1;
	int start_seg = start / 8, start_off = start % 8;
	uint8_t byte = bm->data[start_seg];
	for (int i = start_off; i < 8; i++) {
		if (!((byte >> i) & 1))
			return start_seg * 8 + i;
	}
	int end_seg = (start + len) / 8;
	for (int i = start_seg + 1; i < end_seg; i++) {
		int shift = s_zero_lut[bm->data[i]];
		if (shift >= 0)
			return i * 8 + shift;
	}
	int end_off = (start + len) % 8;
	byte = bm->data[end_seg];
	for (int i = 0; i < end_off; i++) {
		if (!((byte >> i) & 1))
			return end_seg * 8 + i;
	}
	return X_BITMAP_NPOS;
}

static size_t bitmap_find1(x_bitmap *bm, size_t start, size_t len)
{
	if (len == 0)
		return -1;
	int start_seg = start / 8, start_off = start % 8;
	uint8_t byte = bm->data[start_seg];
	for (int i = start_off; i < 8; i++) {
		if ((byte >> i) & 1)
			return start_seg * 8 + i;
	}
	int end_seg = (start + len) / 8;
	for (int i = start_seg + 1; i < end_seg; i++) {
		int shift = s_one_lut[bm->data[i]];
		if (shift >= 0)
			return i * 8 + shift;
	}
	int end_off = (start + len) % 8;
	byte = bm->data[end_seg];
	for (int i = 0; i < end_off; i++) {
		if ((byte >> i) & 1)
			return end_seg * 8 + i;
	}
	return X_BITMAP_NPOS;
}

static size_t bitmap_count0(x_bitmap *bm, size_t start, size_t len)
{
	size_t cnt = 0;
	if (len == 0)
		return -1;
	int start_seg = start / 8, start_off = start % 8;
	uint8_t byte = bm->data[start_seg];
	for (int i = start_off; i < 8; i++) {
		if (!((byte >> i) & 1))
			cnt += 1;
	}
	int end_seg = (start + len) / 8;
	for (int i = start_seg + 1; i < end_seg; i++) {
		cnt +=  8 - s_one_count_lut[bm->data[i]];
	}
	int end_off = (start + len) % 8;
	byte = bm->data[end_seg];
	for (int i = 0; i < end_off; i++) {
		if (!((byte >> i) & 1))
			cnt += 1;
	}
	return cnt;
}

static size_t bitmap_count1(x_bitmap *bm, size_t start, size_t len)
{
	size_t cnt = 0;
	if (len == 0)
		return -1;
	int start_seg = start / 8, start_off = start % 8;
	uint8_t byte = bm->data[start_seg];
	for (int i = start_off; i < 8; i++) {
		if ((byte >> i) & 1)
			cnt += 1;
	}
	int end_seg = (start + len) / 8;
	for (int i = start_seg + 1; i < end_seg; i++) {
		cnt += s_one_count_lut[bm->data[i]];
	}
	int end_off = (start + len) % 8;
	byte = bm->data[end_seg];
	for (int i = 0; i < end_off; i++) {
		if ((byte >> i) & 1)
			cnt += 1;
	}
	return cnt;
}

size_t x_bitmap_find(x_bitmap *bm, int bit, size_t start, size_t len)
{
	x_assert(bm != NULL);
	if (start >= bm->nbytes * 8)
		return X_BITMAP_NPOS;
	if ((start + len) > bm->nbytes * 8)
		len = bm->nbytes * 8 - start;
	return (bit)
		? bitmap_find1(bm, start, len)
		: bitmap_find0(bm, start, len);
}

size_t x_bitmap_count(x_bitmap *bm, int bit, size_t start, size_t len)
{
	x_assert(bm != NULL);
	if (start >= bm->nbytes * 8)
		return X_BITMAP_NPOS;
	if ((start + len) > bm->nbytes * 8)
		len = bm->nbytes * 8 - start;
	return (bit)
		? bitmap_count1(bm, start, len)
		: bitmap_count0(bm, start, len);
}

void x_bitmap_fill(x_bitmap *bm, void *bitarr, size_t start, size_t len)
{
	x_assert(bm != NULL);
	x_assert(bitarr != NULL);
	x_assert_printf(start < bm->nbytes * 8 && (start + len) <= bm->nbytes * 8, "out of bounds");
	uint8_t *src = (uint8_t *)bitarr;
    uint8_t *dst = bm->data;
	if (len == 0)
		return;
    if (start % 8 == 0) {
        size_t dst_byte = start / 8;
        size_t full_bytes = len / 8;
        if (full_bytes > 0)
            memcpy(dst + dst_byte, src, full_bytes);
        size_t rem_bits = len % 8;
        if (rem_bits > 0) {
            uint8_t mask = (1U << rem_bits) - 1;
            dst[dst_byte + full_bytes] &= (uint8_t)~mask;
            dst[dst_byte + full_bytes] |= (src[full_bytes] & mask);
        }
        return;
    }
    for (size_t i = 0; i < len; i++) {
        size_t s_byte = i / 8;
        size_t s_bit = i % 8;
        int bit = (src[s_byte] >> s_bit) & 1;
        size_t d_byte = (start + i) / 8;
        size_t d_bit = (start + i) % 8;
        if (bit)
            dst[d_byte] |= (1U << d_bit);
        else
            dst[d_byte] &= ~(1U << d_bit);
    }
}

