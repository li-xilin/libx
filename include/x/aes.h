/*
 * Copyright (c) 2025 Li Xilin <lixilin@gmx.com>
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
#ifndef X_AES_H
#define X_AES_H

#include "types.h"
#include <stdint.h>
#include <stddef.h>

#define X_AES_BLOCKLEN 16
#define X_AES_KEYEXPSIZE 240

struct x_aes_ctx_st
{
	uint8_t round_key[X_AES_KEYEXPSIZE];
	uint8_t iv[X_AES_BLOCKLEN];
	uint8_t nk, nr;
};

void x_aes_init(x_aes_ctx* ctx, const uint8_t* key, int key_bits);
void x_aes_set_iv(x_aes_ctx* ctx, const uint8_t* iv);
void x_aes_encrypt(const x_aes_ctx* ctx, uint8_t* buf);
void x_aes_decrypt(const x_aes_ctx* ctx, uint8_t* buf);

void x_aes_cbc_encrypt(x_aes_ctx* ctx, uint8_t* buf, size_t length);
void x_aes_cbc_decrypt(x_aes_ctx* ctx, uint8_t* buf, size_t length);
void x_aes_ctr_xcrypt(x_aes_ctx* ctx, uint8_t* buf, size_t length);

#endif

