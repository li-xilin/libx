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
#include "x/endian.h"
#include <stdint.h>

#if X_BYTE_ORDER != X_BIG_ENDIAN && X_BYTE_ORDER != X_LITTLE_ENDIAN
#error "Unknown endian"
#endif

static inline uint64_t reverse64(uint64_t v)
{
	return
		(v & 0xff00000000000000) >> 56 |
		(v & 0x00ff000000000000) >> 40 |
		(v & 0x0000ff0000000000) >> 24 |
		(v & 0x000000ff00000000) >> 8  |
		(v & 0x00000000ff000000) << 8  |
		(v & 0x0000000000ff0000) << 24 |
		(v & 0x000000000000ff00) << 40 |
		(v & 0x00000000000000ff) << 56 ;
}

static inline uint32_t reverse32(uint32_t v)
{
	return
		(v & 0xff000000) >> 24 |
		(v & 0x00ff0000) >> 8  |
		(v & 0x0000ff00) << 8 |
		(v & 0x000000ff) << 24 ;
}

static inline uint16_t reverse16(uint16_t v)
{
	return (v & 0xff00) >> 8 | (v & 0x00ff) << 8;
}

static inline uint32_t pdptole32(uint32_t v)
{
	return
		(v & 0xFF000000) >> 8 |
		(v & 0x00FF0000) << 8 |
		(v & 0x0000FF00) >> 8 |
		(v & 0x000000FF) << 8;
}

uint16_t x_htobe16(uint16_t v)
{
#if X_BYTE_ORDER == X_BIG_ENDIAN
	return v;
#elif X_BYTE_ORDER == X_LITTLE_ENDIAN
	return reverse16(v);
#endif
}

uint16_t x_htole16(uint16_t v)
{
#if X_BYTE_ORDER == X_BIG_ENDIAN
	return reverse16(v);
#elif X_BYTE_ORDER == X_LITTLE_ENDIAN
	return v;
#endif
}

uint32_t x_htobe32(uint32_t v)
{
#if X_BYTE_ORDER == X_BIG_ENDIAN
	return v;
#elif X_BYTE_ORDER == X_LITTLE_ENDIAN
	return reverse32(v);
#endif
}

uint32_t x_htole32(uint32_t v)
{
#if X_BYTE_ORDER == X_BIG_ENDIAN
	return reverse32(v);
#elif X_BYTE_ORDER == X_LITTLE_ENDIAN
	return v;
#endif
}

uint64_t x_htobe64(uint64_t v)
{
#if X_BYTE_ORDER == X_BIG_ENDIAN
	return v;
#elif X_BYTE_ORDER == X_LITTLE_ENDIAN
	return reverse64(v);
#endif
}

uint64_t x_htole64(uint64_t v)
{
#if X_BYTE_ORDER == X_BIG_ENDIAN
	return reverse64(v);
#elif X_BYTE_ORDER == X_LITTLE_ENDIAN
	return v;
#endif
}

uint16_t x_be16toh(uint16_t v)
{
	return x_htobe16(v);
}

uint16_t x_le16toh(uint16_t v)
{
	return x_htole16(v);
}

uint32_t x_be32toh(uint32_t v)
{
	return x_htobe32(v);
}

uint32_t x_le32toh(uint32_t v)
{
	return x_htole32(v);
}

uint64_t x_be64toh(uint64_t v)
{
	return x_htobe64(v);
}

uint64_t x_le64toh(uint64_t v)
{
	return x_htole64(v);
}

uint32_t x_pdp32toh(uint32_t v)
{
#if X_BYTE_ORDER == X_BIG_ENDIAN
	return reverse32(pdptole32(v));
#elif X_BYTE_ORDER == X_LITTLE_ENDIAN
	return pdptole32(v);
#endif
}

