#include "x/sha256.h"
#include <string.h>

#define ROR(value, bits) (((value) >> (bits)) | ((value) << (32 - (bits))))
#define MIN(x, y) (((x) < (y)) ? (x) : (y))
#define STORE32H(x, y) \
{ \
	(y)[0] = (uint8_t)(((x) >> 24) & 255); \
	(y)[1] = (uint8_t)(((x) >> 16) & 255); \
	(y)[2] = (uint8_t)(((x) >> 8) & 255); \
	(y)[3] = (uint8_t)((x)&255); \
}
#define LOAD32H(x, y) \
{ \
	x = ((uint32_t)((y)[0] & 255) << 24) | ((uint32_t)((y)[1] & 255) << 16) | \
	((uint32_t)((y)[2] & 255) << 8) | ((uint32_t)((y)[3] & 255)); \
}
#define STORE64H(x, y) \
{ \
	(y)[0] = (uint8_t)(((x) >> 56) & 255); \
	(y)[1] = (uint8_t)(((x) >> 48) & 255); \
	(y)[2] = (uint8_t)(((x) >> 40) & 255); \
	(y)[3] = (uint8_t)(((x) >> 32) & 255); \
	(y)[4] = (uint8_t)(((x) >> 24) & 255); \
	(y)[5] = (uint8_t)(((x) >> 16) & 255); \
	(y)[6] = (uint8_t)(((x) >> 8) & 255); \
	(y)[7] = (uint8_t)((x)&255); \
}
#define SHA256_ROUND(a, b, c, d, e, f, g, h, i) \
{ \
	t0 = h + SIGMA1(e) + CH(e, f, g) + K[i] + W[i]; \
	t1 = SIGMA0(a) + MAJ(a, b, c); \
	d += t0; \
	h = t0 + t1; \
}
#define CH(x, y, z) (z ^ (x & (y ^ z)))
#define MAJ(x, y, z) (((x | y) & z) | (x & y))
#define S(x, n) ROR((x), (n))
#define R(x, n) (((x)&0xFFFFFFFFUL) >> (n))
#define SIGMA0(x) (S(x, 2) ^ S(x, 13) ^ S(x, 22))
#define SIGMA1(x) (S(x, 6) ^ S(x, 11) ^ S(x, 25))
#define GAMMA0(x) (S(x, 7) ^ S(x, 18) ^ R(x, 3))
#define GAMMA1(x) (S(x, 17) ^ S(x, 19) ^ R(x, 10))
#define BLOCK_SIZE 64

static const uint32_t K[64] = {
	0x428a2f98UL, 0x71374491UL, 0xb5c0fbcfUL, 0xe9b5dba5UL, 0x3956c25bUL,
	0x59f111f1UL, 0x923f82a4UL, 0xab1c5ed5UL, 0xd807aa98UL, 0x12835b01UL,
	0x243185beUL, 0x550c7dc3UL, 0x72be5d74UL, 0x80deb1feUL, 0x9bdc06a7UL,
	0xc19bf174UL, 0xe49b69c1UL, 0xefbe4786UL, 0x0fc19dc6UL, 0x240ca1ccUL,
	0x2de92c6fUL, 0x4a7484aaUL, 0x5cb0a9dcUL, 0x76f988daUL, 0x983e5152UL,
	0xa831c66dUL, 0xb00327c8UL, 0xbf597fc7UL, 0xc6e00bf3UL, 0xd5a79147UL,
	0x06ca6351UL, 0x14292967UL, 0x27b70a85UL, 0x2e1b2138UL, 0x4d2c6dfcUL,
	0x53380d13UL, 0x650a7354UL, 0x766a0abbUL, 0x81c2c92eUL, 0x92722c85UL,
	0xa2bfe8a1UL, 0xa81a664bUL, 0xc24b8b70UL, 0xc76c51a3UL, 0xd192e819UL,
	0xd6990624UL, 0xf40e3585UL, 0x106aa070UL, 0x19a4c116UL, 0x1e376c08UL,
	0x2748774cUL, 0x34b0bcb5UL, 0x391c0cb3UL, 0x4ed8aa4aUL, 0x5b9cca4fUL,
	0x682e6ff3UL, 0x748f82eeUL, 0x78a5636fUL, 0x84c87814UL, 0x8cc70208UL,
	0x90befffaUL, 0xa4506cebUL, 0xbef9a3f7UL, 0xc67178f2UL };

static void transform(x_sha256_ctx* ctx, uint8_t const* buf)
{
	uint32_t S[8], W[64], t0, t1, t;
	// Copy state into S
	for (int i = 0; i < 8; i++)
		S[i] = ctx->state[i];
	// Copy the state into 512-bits into W[0..15]
	for (int i = 0; i < 16; i++)
		LOAD32H(W[i], buf + (4 * i));
	// Fill W[16..63]
	for (int i = 16; i < 64; i++)
		W[i] = GAMMA1(W[i - 2]) + W[i - 7] + GAMMA0(W[i - 15]) + W[i - 16];
	// Compress
	for (int i = 0; i < 64; i++) {
		SHA256_ROUND(S[0], S[1], S[2], S[3], S[4], S[5], S[6], S[7], i);
		t = S[7];
		S[7] = S[6];
		S[6] = S[5];
		S[5] = S[4];
		S[4] = S[3];
		S[3] = S[2];
		S[2] = S[1];
		S[1] = S[0];
		S[0] = t;
	}
	// Feedback
	for (int i = 0; i < 8; i++)
		ctx->state[i] = ctx->state[i] + S[i];
}

void x_sha256_init(x_sha256_ctx* ctx)
{
	ctx->curlen = 0;
	ctx->length = 0;
	ctx->state[0] = 0x6A09E667UL;
	ctx->state[1] = 0xBB67AE85UL;
	ctx->state[2] = 0x3C6EF372UL;
	ctx->state[3] = 0xA54FF53AUL;
	ctx->state[4] = 0x510E527FUL;
	ctx->state[5] = 0x9B05688CUL;
	ctx->state[6] = 0x1F83D9ABUL;
	ctx->state[7] = 0x5BE0CD19UL;
}

void x_sha256_update(x_sha256_ctx* ctx, void const* buf, uint32_t buf_size)
{
	uint32_t n;
	if (ctx->curlen > sizeof(ctx->buf))
		return;
	while (buf_size > 0) {
		if (ctx->curlen == 0 && buf_size >= BLOCK_SIZE) {
			transform(ctx, (uint8_t*)buf);
			ctx->length += BLOCK_SIZE * 8;
			buf = (uint8_t*)buf + BLOCK_SIZE;
			buf_size -= BLOCK_SIZE;
		}
		else {
			n = MIN(buf_size, (BLOCK_SIZE - ctx->curlen));
			memcpy(ctx->buf + ctx->curlen, buf, (size_t)n);
			ctx->curlen += n;
			buf = (uint8_t*)buf + n;
			buf_size -= n;
			if (ctx->curlen == BLOCK_SIZE) {
				transform(ctx, ctx->buf);
				ctx->length += 8 * BLOCK_SIZE;
				ctx->curlen = 0;
			}
		}
	}
}

void x_sha256_finish(x_sha256_ctx* ctx, uint8_t* digest)
{
	if (ctx->curlen >= sizeof(ctx->buf))
		return;
	// Increase the length of the message
	ctx->length += ctx->curlen * 8;
	// Append the '1' bit
	ctx->buf[ctx->curlen++] = (uint8_t)0x80;
	// if the length is currently above 56 bytes we append zeros
	// then compress. Then we can fall back to padding zeros and length
	// encoding like normal.
	if (ctx->curlen > 56) {
		while (ctx->curlen < 64)
			ctx->buf[ctx->curlen++] = (uint8_t)0;
		transform(ctx, ctx->buf);
		ctx->curlen = 0;
	}
	// Pad up to 56 bytes of zeroes
	while (ctx->curlen < 56)
		ctx->buf[ctx->curlen++] = (uint8_t)0;
	// Store length
	STORE64H(ctx->length, ctx->buf + 56);
	transform(ctx, ctx->buf);
	// Copy output
	for (int i = 0; i < 8; i++)
		STORE32H(ctx->state[i], digest + (4 * i));
}

void x_sha256_buffer(void const* buf, uint32_t buf_size, uint8_t* digest)
{
	x_sha256_ctx context;
	x_sha256_init(&context);
	x_sha256_update(&context, buf, buf_size);
	x_sha256_finish(&context, digest);
}
