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
#include "x/md5.h"
#include <stdio.h>

static uint32_t S[] = {
	7, 12, 17, 22, 7, 12, 17, 22, 7, 12, 17, 22, 7, 12, 17, 22,
	5,  9, 14, 20, 5,  9, 14, 20, 5,  9, 14, 20, 5,  9, 14, 20,
	4, 11, 16, 23, 4, 11, 16, 23, 4, 11, 16, 23, 4, 11, 16, 23,
	6, 10, 15, 21, 6, 10, 15, 21, 6, 10, 15, 21, 6, 10, 15, 21};

static uint32_t K[] = {
	0xd76aa478, 0xe8c7b756, 0x242070db, 0xc1bdceee,
	0xf57c0faf, 0x4787c62a, 0xa8304613, 0xfd469501,
	0x698098d8, 0x8b44f7af, 0xffff5bb1, 0x895cd7be,
	0x6b901122, 0xfd987193, 0xa679438e, 0x49b40821,
	0xf61e2562, 0xc040b340, 0x265e5a51, 0xe9b6c7aa,
	0xd62f105d, 0x02441453, 0xd8a1e681, 0xe7d3fbc8,
	0x21e1cde6, 0xc33707d6, 0xf4d50d87, 0x455a14ed,
	0xa9e3e905, 0xfcefa3f8, 0x676f02d9, 0x8d2a4c8a,
	0xfffa3942, 0x8771f681, 0x6d9d6122, 0xfde5380c,
	0xa4beea44, 0x4bdecfa9, 0xf6bb4b60, 0xbebfbc70,
	0x289b7ec6, 0xeaa127fa, 0xd4ef3085, 0x04881d05,
	0xd9d4d039, 0xe6db99e5, 0x1fa27cf8, 0xc4ac5665,
	0xf4292244, 0x432aff97, 0xab9423a7, 0xfc93a039,
	0x655b59c3, 0x8f0ccc92, 0xffeff47d, 0x85845dd1,
	0x6fa87e4f, 0xfe2ce6e0, 0xa3014314, 0x4e0811a1,
	0xf7537e82, 0xbd3af235, 0x2ad7d2bb, 0xeb86d391};

static uint8_t PADDING[] = {
	0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

#define F(X, Y, Z) ((X & Y) | (~X & Z))
#define G(X, Y, Z) ((X & Z) | (Y & ~Z))
#define H(X, Y, Z) (X ^ Y ^ Z)
#define I(X, Y, Z) (Y ^ (X | ~Z))

static uint32_t rotate_left(uint32_t x, uint32_t n)
{
	return (x << n) | (x >> (32 - n));
}

static void step(uint32_t buffer[4], uint32_t input[16])
{
	uint32_t AA = buffer[0];
	uint32_t BB = buffer[1];
	uint32_t CC = buffer[2];
	uint32_t DD = buffer[3];
	uint32_t E;
	size_t j;
	for(size_t i = 0; i < 64; ++i) {
		switch(i / 16) {
			case 0:
				E = F(BB, CC, DD);
				j = i;
				break;
			case 1:
				E = G(BB, CC, DD);
				j = ((i * 5) + 1) % 16;
				break;
			case 2:
				E = H(BB, CC, DD);
				j = ((i * 3) + 5) % 16;
				break;
			default:
				E = I(BB, CC, DD);
				j = (i * 7) % 16;
				break;
		}

		uint32_t temp = DD;
		DD = CC;
		CC = BB;
		BB = BB + rotate_left(AA + E + K[i] + input[j], S[i]);
		AA = temp;
	}
	buffer[0] += AA;
	buffer[1] += BB;
	buffer[2] += CC;
	buffer[3] += DD;
}


void x_md5_init(x_md5_ctx *ctx)
{
	ctx->size = 0;
	uint32_t tmp[4] = X_MD5_ABCD;
	ctx->buffer[0] = tmp[0];
	ctx->buffer[1] = tmp[1];
	ctx->buffer[2] = tmp[2];
	ctx->buffer[3] = tmp[3];
}

void x_md5_update(x_md5_ctx *ctx, const void *data, size_t size)
{
	uint32_t input[16];
	size_t offset = ctx->size % 64;
	ctx->size += (uint64_t)size;
	for(size_t i = 0; i < size; ++i) {
		ctx->input[offset++] = ((uint8_t *)data)[i];
		if(offset % 64 == 0) {
			for(size_t j = 0; j < 16; ++j) {
				input[j] = (uint32_t)(ctx->input[(j * 4) + 3]) << 24 |
					(uint32_t)(ctx->input[(j * 4) + 2]) << 16 |
					(uint32_t)(ctx->input[(j * 4) + 1]) <<  8 |
					(uint32_t)(ctx->input[(j * 4)]);
			}
			step(ctx->buffer, input);
			offset = 0;
		}
	}
}

void x_md5_finish(x_md5_ctx *ctx, x_md5 *md5)
{
	uint32_t input[16];
	size_t offset = ctx->size % 64;
	size_t padding_length = offset < 56 ? 56 - offset : (56 + 64) - offset;
	x_md5_update(ctx, PADDING, padding_length);
	ctx->size -= (uint64_t)padding_length;
	for(size_t j = 0; j < 14; ++j) {
		input[j] = (uint32_t)(ctx->input[(j * 4) + 3]) << 24 |
			(uint32_t)(ctx->input[(j * 4) + 2]) << 16 |
			(uint32_t)(ctx->input[(j * 4) + 1]) <<  8 |
			(uint32_t)(ctx->input[(j * 4)]);
	}
	input[14] = (uint32_t)(ctx->size * 8);
	input[15] = (uint32_t)((ctx->size * 8) >> 32);
	step(ctx->buffer, input);
	for(size_t i = 0; i < 4; ++i) {
		md5->u8[(i * 4) + 0] = (uint8_t)((ctx->buffer[i] & 0x000000FF));
		md5->u8[(i * 4) + 1] = (uint8_t)((ctx->buffer[i] & 0x0000FF00) >>  8);
		md5->u8[(i * 4) + 2] = (uint8_t)((ctx->buffer[i] & 0x00FF0000) >> 16);
		md5->u8[(i * 4) + 3] = (uint8_t)((ctx->buffer[i] & 0xFF000000) >> 24);
	}
}

void x_md5_buffer(void *data, size_t len, x_md5 *result)
{
	x_md5_ctx ctx;
	x_md5_init(&ctx);
	x_md5_update(&ctx, data, len);
	x_md5_finish(&ctx, result);
}

int x_md5_file(void *fp, x_md5 *result)
{
	char buf[1024];
	size_t len = 0;
	x_md5_ctx ctx;
	x_md5_init(&ctx);
	while((len = fread(buf, 1, sizeof buf, fp)) > 0)
		x_md5_update(&ctx, (uint8_t *)buf, len);
	if (ferror((FILE *)fp))
		return -1;
	x_md5_finish(&ctx, result);
	return 0;
}

