#ifndef X_SHA256_H
#define X_SHA256_H

#include "types.h"
#include <stdint.h>
#include <stdio.h>

#define X_SHA256_HASH_SIZE (256 / 8)

struct x_sha256_ctx_st {
	uint64_t length;
	uint32_t state[8];
	uint32_t curlen;
	uint8_t buf[64];
};

void x_sha256_init(x_sha256_ctx* ctx);
void x_sha256_update(x_sha256_ctx* ctx, void const* buf, uint32_t buf_size);
void x_sha256_finish(x_sha256_ctx* ctx, uint8_t* digest);
void x_sha256_buffer(void const* buf, uint32_t buf_size, uint8_t* digest);

#endif

