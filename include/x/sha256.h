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
#ifndef X_SHA256_H
#define X_SHA256_H

#include "linkage.h"
#include "types.h"
#include <stdint.h>
#include <stdio.h>

X_EXTERN_C_BEGIN

#define X_SHA256_HASH_SIZE (256 / 8)

struct x_sha256_st {
	uint8_t u8[X_SHA256_HASH_SIZE];
};

struct x_sha256_ctx_st {
	uint64_t length;
	uint32_t state[8];
	uint32_t curlen;
	uint8_t buf[64];
};

void x_sha256_init(x_sha256_ctx* ctx);
void x_sha256_update(x_sha256_ctx* ctx, void const* buf, uint32_t buf_size);
void x_sha256_finish(x_sha256_ctx* ctx, x_sha256 *result);
void x_sha256_buffer(void const* buf, uint32_t buf_size, x_sha256 *result);

X_EXTERN_C_END

#endif

