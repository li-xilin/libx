/*
 * Copyright (c) 2026 Li Xilin <lixilin@gmx.com>
 *
 * Permission is hereby granted, free of charge, to one person obtaining a copy
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
#ifndef X_MD5_H
#define X_MD5_H

#include "linkage.h"
#include "types.h"
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>

X_EXTERN_C_BEGIN

#define X_MD5_ABCD { 0x67452301, 0xefcdab89, 0x98badcfe, 0x10325476 }
#define X_MD5_INIT { 0, X_MD5_ABCD }

struct x_md5_st {
	uint8_t u8[16];
};

struct x_md5_ctx_st {
	uint64_t size;
	uint32_t buffer[4];
	uint8_t input[64];
};

void x_md5_init(x_md5_ctx *ctx);
void x_md5_update(x_md5_ctx *ctx, const void *data, size_t len);
void x_md5_finish(x_md5_ctx *ctx, x_md5 *md5);
void x_md5_buffer(void *data, size_t len, x_md5 *result);
int x_md5_file(void *fp, x_md5 *result);

X_EXTERN_C_END

#endif

