/*
 * Copyright (c) 2024,2025 Li Xilin <lixilin@gmx.com>
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

#ifndef X_BITMAP_H
#define X_BITMAP_H

#include "types.h"
#include "assert.h"
#include <stdint.h>
#include <string.h>

struct x_bitmap_st
{
	size_t nbytes;
	uint8_t *data;
};

inline static void x_bitmap_init(x_bitmap *bm, void *data, size_t size)
{
	bm->data = data;
	bm->nbytes = size;
}

inline static size_t x_bitmap_nbits(x_bitmap *bm)
{
	return bm->nbytes * 8;
}

inline static int x_bitmap_get(x_bitmap *bm, size_t idx)
{
	x_assert(idx < bm->nbytes * 8, "Index out of bounds");
	int byte_idx = idx / 8, bit_idx = idx % 8;
	return (bm->data[byte_idx] >> bit_idx) & 1;
}

inline static void x_bitmap_set(x_bitmap *bm, size_t idx, int bit)
{
	x_assert(idx < bm->nbytes * 8, "Index out of bounds");
	int byte_idx = idx / 8, bit_idx = idx % 8;
	uint8_t old_bit = (bm->data[byte_idx] >> bit_idx) & 1;
	bm->data[byte_idx] ^= (old_bit ^ !!bit) << bit_idx;
}

inline static void x_bitmap_clear(x_bitmap *bm, int val)
{
	memset(bm->data, !!val * 0xFF, bm->nbytes);
}

inline static void x_bitmap_and(x_bitmap *bm, size_t idx, int bit)
{
	x_assert(idx < bm->nbytes * 8, "Index out of bounds");
	int byte_idx = idx / 8, bit_idx = idx % 8;
	bm->data[byte_idx] &=  ~(!bit << bit_idx);
}

inline static void x_bitmap_or(x_bitmap *bm, size_t idx, int bit)
{
	x_assert(idx < bm->nbytes * 8, "Index out of bounds");
	int byte_idx = idx / 8, bit_idx = idx % 8;
	bm->data[byte_idx] |=  !!bit << bit_idx;
}

inline static void x_bitmap_toggle(x_bitmap *bm, size_t idx)
{
	x_assert(idx < bm->nbytes * 8, "Index out of bounds");
	int byte_idx = idx / 8, bit_idx = idx % 8;
	bm->data[byte_idx] ^=  (1 << bit_idx);
}

int x_bitmap_find(x_bitmap *bm, int bit, size_t start, size_t len);
size_t x_bitmap_count(x_bitmap *bm);

#endif
