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
#ifndef X_ENDIAN_H
#define X_ENDIAN_H

#include "detect.h"
#include "linkage.h"
#include <stdint.h>

X_EXTERN_C_BEGIN

/* Detect machine byte-order */
#if defined(X_OS_WIN)
#  include <sys/param.h>
#elif defined(X_OS_MACOS)
#    include <libkern/OSByteOrder.h>
#elif defined(X_OS_LINUX) ||  defined(X_OS_CYGWIN)
#  include <endian.h>
#else
#  include <sys/endian.h>
#endif
#if defined(__BYTE_ORDER__)
#  define X_BYTE_ORDER __BYTE_ORDER__
#elif defined(__BYTE_ORDER)
#  define X_BYTE_ORDER __BYTE_ORDER
#elif defined(BYTE_ORDER)
#  define X_BYTE_ORDER BYTE_ORDER
#else
#  error "Unknown byte-order"
#endif
#if defined(__ORDER_BIG_ENDIAN__)
#  define X_BIG_ENDIAN __ORDER_BIG_ENDIAN__
#elif defined(__BIG_ENDIAN)
#  define X_BIG_ENDIAN __BIG_ENDIAN
#else
#  define X_BIG_ENDIAN 4321
#endif
#if defined(__ORDER_LITTLE_ENDIAN__)
#  define X_LITTLE_ENDIAN __ORDER_LITTLE_ENDIAN__
#elif defined(__LITTLE_ENDIAN)
#  define X_LITTLE_ENDIAN __LITTLE_ENDIAN
#else
#  define X_LITTLE_ENDIAN 1234
#endif
#if defined(__ORDER_PDP_ENDIAN__)
#  define X_PDP_ENDIAN __ORDER_PDP_ENDIAN__
#elif defined(__ORDER_PDP_ENDIAN)
#  define X_PDP_ENDIAN __ORDER_PDP_ENDIAN
#else
#  define X_PDP_ENDIAN 3412
#endif

uint16_t x_htobe16(uint16_t v);
uint16_t x_htole16(uint16_t v);
uint32_t x_htobe32(uint32_t v);
uint32_t x_htole32(uint32_t v);
uint64_t x_htobe64(uint64_t v);
uint64_t x_htole64(uint64_t v);
uint16_t x_be16toh(uint16_t v);
uint16_t x_le16toh(uint16_t v);
uint32_t x_be32toh(uint32_t v);
uint32_t x_le32toh(uint32_t v);
uint64_t x_be64toh(uint64_t v);
uint64_t x_le64toh(uint64_t v);
uint32_t x_pdp32toh(uint32_t v);

X_EXTERN_C_END

#endif
