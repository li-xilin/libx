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
#include "x/uuid.h"
#include "x/random.h"
#include "x/errno.h"
#include "x/time.h"
#include "x/once.h"
#include "x/endian.h"
#include <stdint.h>
#include <string.h>
#include <stdio.h>

#define UUID_EPOCH_OFFSET 0x01B21DD213814000ULL /* 1582-10-15 -> 1970-01-01 100ns count */

static uint8_t  s_node[6]      = {0};
static uint16_t s_clock_seq    = 0;
static uint64_t s_last_ts      = 0;
static x_once s_init_once = X_ONCE_INIT;

static void uuid_v1_init(void)
{
    uint16_t seq_tmp;
    x_random(s_node, sizeof(s_node));
    x_random(&seq_tmp, sizeof(seq_tmp));
    s_node[0] |= 0x01;
    s_clock_seq = seq_tmp & 0x3FFF;
}

static int uuidv1_timestamp(uint64_t *out)
{
    struct timeval tv;
    if (x_time_now(&tv) != 0)
        return -1;
    *out = ((uint64_t)tv.tv_sec * 10000000ULL)
           + ((uint64_t)tv.tv_usec * 10ULL)
           + UUID_EPOCH_OFFSET;
    return 0;
}

int x_uuid_gen_v1(x_uuid *uuid)
{
	struct uuid_v1 {
		uint32_t time_low;
		uint16_t time_mid;
		uint16_t time_hi_and_version;
		uint8_t clock_seq_hi_and_reserved;
		uint8_t clock_seq_low;
		uint8_t	node[6];
	} *tmp = (struct uuid_v1 *)uuid;
	uint64_t ts;
	if (uuid == NULL) {
		errno = X_EINVAL;
		return -1;
	}
	x_once_init(&s_init_once, uuid_v1_init);
	if (uuidv1_timestamp(&ts))
		return -1;
	if (ts <= s_last_ts)
		s_clock_seq = (s_clock_seq + 1) & 0x3FFF;
	s_last_ts = ts;
	tmp->time_low = x_htobe32((uint32_t)(ts & 0xFFFFFFFF));
	tmp->time_mid = x_htobe16((uint16_t)((ts >> 32) & 0xFFFF));
	tmp->time_hi_and_version = x_htobe16((1 << 12) | ((uint16_t)((ts >> 48)) & 0x0FFF));
	tmp->clock_seq_low = s_clock_seq & 0xFF;
	tmp->clock_seq_hi_and_reserved = (s_clock_seq & 0x3F00) >> 8;
	tmp->clock_seq_hi_and_reserved |= 0x80;
	memcpy(tmp->node, s_node, 6);
	return 0;
}

int x_uuid_gen_v4(x_uuid *uuid)
{
	if (x_random(uuid->u8, sizeof uuid->u8))
		return -1;
	uuid->u8[6] = (uuid->u8[6] & 0x0F) | 0x40;
	uuid->u8[8] = (uuid->u8[8] & 0x3F) | 0x80;
	return 0;
}

int x_uuid_gen_v7(x_uuid *uuid)
{
	if (uuid == NULL) {
		errno = X_EINVAL;
		return -1;
	}
	struct timeval tv;
	if (x_time_now(&tv) != 0)
		return -1;
	char ran[10];
	if (x_random(ran, sizeof ran))
		return -1;
	uint64_t ts = tv.tv_sec * 1000 + tv.tv_usec / 1000;
	uuid->u8[0] = (uint8_t)(ts >> 40);
	uuid->u8[1] = (uint8_t)(ts >> 32);
	uuid->u8[2] = (uint8_t)(ts >> 24);
	uuid->u8[3] = (uint8_t)(ts >> 16);
	uuid->u8[4] = (uint8_t)(ts >> 8);
	uuid->u8[5] = (uint8_t)ts;
	memcpy(uuid->u8 + 6, ran, 2);
	uuid->u8[6] &= 0x0F;
	uuid->u8[6] |= 0x70;
	memcpy(uuid->u8 + 8, ran + 2, 8);
	uuid->u8[8] &= 0x3F;
	uuid->u8[8] |= 0x80;
	return 0;
}

static inline int hexval(char c)
{
	if (c >= '0' && c <= '9')
		return c - '0';
	if (c >= 'a' && c <= 'f')
		return c - 'a' + 10;
	if (c >= 'A' && c <= 'F')
		return c - 'A' + 10;
	return -1;
}

void x_uuid_text(const x_uuid *uuid, char *out)
{
	static const char hex[] = "0123456789abcdef";
	int j = 0;
	for (int i = 0; i < 16; i++) {
		if (i == 4 || i == 6 || i == 8 || i == 10) {
			out[j++] = '-';
		}
		out[j++] = hex[(uuid->u8[i] >> 4) & 0x0F];
		out[j++] = hex[uuid->u8[i] & 0x0F];
	}
	out[j] = '\0';
}

int x_uuid_parse(const char *text, x_uuid *out)
{
	if (text == NULL || out == NULL)
		goto err;
	size_t len = strlen(text);
	if (len != 32 && len != 36)
		goto err;
	int nibble_count = 0;
	for (size_t i = 0; i < len; i++) {
		char c = text[i];
		if (c == '-') {
			if (len != 36 || (i != 8 && i != 13 && i != 18 && i != 23))
				goto err;
			continue;
		}
		int v = hexval(c);
		if (v < 0)
			goto err;
		if (nibble_count & 1)
			out->u8[nibble_count >> 1] |= (uint8_t)v;
		else
			out->u8[nibble_count >> 1] = (uint8_t)(v << 4);
		nibble_count++;
	}
	if (nibble_count != 32)
		goto err;
	return 0;
err:
	errno = X_EINVAL;
	return -1;
}

