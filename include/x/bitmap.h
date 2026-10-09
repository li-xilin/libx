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

#include "linkage.h"
#include "types.h"
#include "assert.h"
#include <string.h>

X_EXTERN_C_BEGIN

#if defined(X_CC_MSVC)
	#include <intrin.h>
	static inline size_t __x_msvc_ctzll(uint64_t x)
	{
		unsigned long index = 0;
		if (_BitScanForward64(&index, x))
			return index;
		return 64;
	}
	#define X_BITMAP64_FIRST_ONE(s) __x_msvc_ctzll((uint64_t)s);
	#define X_BITMAP64_ONE_COUNT(s) __popcnt64(s)
#elif defined(X_CC_GNU) || defined(X_CC_CLANG)
	#define X_BITMAP64_FIRST_ONE(s) ((size_t)__builtin_ctzll((unsigned long long)(s)))
	#define X_BITMAP64_ONE_COUNT(s) ((size_t)__builtin_popcountll((unsigned long long)(s)))
#else
	static inline size_t __x_fallback_popcountll(unsigned long long x)
	{
		size_t count = 0;
		while (x) {
			x &= (x - 1);
			count++;
		}
		return count;
	}
	static inline size_t __x_fallback_ctzll(unsigned long long x)
	{
		if (x == 0) return 64;
		size_t count = 0;
		while ((x & 1ULL) == 0) {
			x >>= 1;
			count++;
		}
		return count;
	}
	#define X_BITMAP64_FIRST_ONE(s) __x_fallback_ctzll((unsigned long long)(s))
	#define X_BITMAP64_ONE_COUNT(s) __x_fallback_popcountll((unsigned long long)(s))
#endif

#define X_BITMAP_NPOS ((size_t)-1)
#define X_BITMAP_INIT(_buf, _size) { (_size), (uint8_t *)(_buf) }

struct x_bitmap_st
{
	size_t nbytes;
	uint8_t *data;
};

static inline void x_bitmap_init(x_bitmap *bm, void *data, size_t size)
{
	bm->data = data;
	bm->nbytes = size;
}

static inline size_t x_bitmap_nbits(x_bitmap *bm)
{
	return bm->nbytes * 8;
}

static inline int x_bitmap_get(x_bitmap *bm, size_t idx)
{
	x_assert_printf(idx < bm->nbytes * 8, "Index out of bounds");
	int byte_idx = idx / 8, bit_idx = idx % 8;
	return (bm->data[byte_idx] >> bit_idx) & 1;
}

static inline void x_bitmap_set(x_bitmap *bm, size_t idx, int bit)
{
	x_assert_printf(idx < bm->nbytes * 8, "Index out of bounds");
	int byte_idx = idx / 8, bit_idx = idx % 8;
	uint8_t old_bit = (bm->data[byte_idx] >> bit_idx) & 1;
	bm->data[byte_idx] ^= (old_bit ^ !!bit) << bit_idx;
}

static inline void x_bitmap_clear(x_bitmap *bm, int bit)
{
	memset(bm->data, !!bit * 0xFF, bm->nbytes);
}

static inline void x_bitmap_and(x_bitmap *bm, size_t idx, int bit)
{
	x_assert_printf(idx < bm->nbytes * 8, "Index out of bounds");
	int byte_idx = idx / 8, bit_idx = idx % 8;
	if (!bit)
		bm->data[byte_idx] &=  ~(1 << bit_idx);
}

static inline void x_bitmap_or(x_bitmap *bm, size_t idx, int bit)
{
	x_assert_printf(idx < bm->nbytes * 8, "Index out of bounds");
	int byte_idx = idx / 8, bit_idx = idx % 8;
	if (bit)
		bm->data[byte_idx] |=  1 << bit_idx;
}

static inline void x_bitmap_xor(x_bitmap *bm, size_t idx, int bit)
{
	x_assert_printf(idx < bm->nbytes * 8, "Index out of bounds");
	int byte_idx = idx / 8, bit_idx = idx % 8;
	if (bit)
		bm->data[byte_idx] ^=  1 << bit_idx;
}

static inline void x_bitmap_not(x_bitmap *bm, size_t idx)
{
	x_assert_printf(idx < bm->nbytes * 8, "Index out of bounds");
	int byte_idx = idx / 8, bit_idx = idx % 8;
	bm->data[byte_idx] ^=  1 << bit_idx;
}

size_t x_bitmap_find(x_bitmap *bm, int bit, size_t start, size_t len);
size_t x_bitmap_count(x_bitmap *bm, int bit, size_t start, size_t len);
void x_bitmap_fill(x_bitmap *bm, void *bitarr, size_t start, size_t len);

X_EXTERN_C_END

#endif

