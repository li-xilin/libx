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
#ifndef X_UUID_H
#define X_UUID_H

#include "string.h"
#include "types.h"
#include "linkage.h"

X_EXTERN_C_BEGIN

#define X_UUID_INIT_RAW(str) {{ \
	(X_UNHEXLIFY_CHAR(str[ 0]) << 4) | (X_UNHEXLIFY_CHAR(str[ 1]) ), \
	(X_UNHEXLIFY_CHAR(str[ 2]) << 4) | (X_UNHEXLIFY_CHAR(str[ 3]) ), \
	(X_UNHEXLIFY_CHAR(str[ 4]) << 4) | (X_UNHEXLIFY_CHAR(str[ 5]) ), \
	(X_UNHEXLIFY_CHAR(str[ 6]) << 4) | (X_UNHEXLIFY_CHAR(str[ 7]) ), \
	(X_UNHEXLIFY_CHAR(str[ 8]) << 4) | (X_UNHEXLIFY_CHAR(str[ 9]) ), \
	(X_UNHEXLIFY_CHAR(str[10]) << 4) | (X_UNHEXLIFY_CHAR(str[11]) ), \
	(X_UNHEXLIFY_CHAR(str[12]) << 4) | (X_UNHEXLIFY_CHAR(str[13]) ), \
	(X_UNHEXLIFY_CHAR(str[14]) << 4) | (X_UNHEXLIFY_CHAR(str[15]) ), \
	(X_UNHEXLIFY_CHAR(str[16]) << 4) | (X_UNHEXLIFY_CHAR(str[17]) ), \
	(X_UNHEXLIFY_CHAR(str[18]) << 4) | (X_UNHEXLIFY_CHAR(str[19]) ), \
	(X_UNHEXLIFY_CHAR(str[20]) << 4) | (X_UNHEXLIFY_CHAR(str[21]) ), \
	(X_UNHEXLIFY_CHAR(str[22]) << 4) | (X_UNHEXLIFY_CHAR(str[23]) ), \
	(X_UNHEXLIFY_CHAR(str[24]) << 4) | (X_UNHEXLIFY_CHAR(str[25]) ), \
	(X_UNHEXLIFY_CHAR(str[26]) << 4) | (X_UNHEXLIFY_CHAR(str[27]) ), \
	(X_UNHEXLIFY_CHAR(str[28]) << 4) | (X_UNHEXLIFY_CHAR(str[29]) ), \
	(X_UNHEXLIFY_CHAR(str[30]) << 4) | (X_UNHEXLIFY_CHAR(str[31]) ), \
}}

#define X_UUID_INIT(str) {{ \
	(X_UNHEXLIFY_CHAR(str[ 0 + 0]) << 4) | (X_UNHEXLIFY_CHAR(str[ 1 + 0]) ), \
	(X_UNHEXLIFY_CHAR(str[ 2 + 0]) << 4) | (X_UNHEXLIFY_CHAR(str[ 3 + 0]) ), \
	(X_UNHEXLIFY_CHAR(str[ 4 + 0]) << 4) | (X_UNHEXLIFY_CHAR(str[ 5 + 0]) ), \
	(X_UNHEXLIFY_CHAR(str[ 6 + 0]) << 4) | (X_UNHEXLIFY_CHAR(str[ 7 + 0]) ), \
	(X_UNHEXLIFY_CHAR(str[ 8 + 1]) << 4) | (X_UNHEXLIFY_CHAR(str[ 9 + 1]) ), \
	(X_UNHEXLIFY_CHAR(str[10 + 1]) << 4) | (X_UNHEXLIFY_CHAR(str[11 + 1]) ), \
	(X_UNHEXLIFY_CHAR(str[12 + 2]) << 4) | (X_UNHEXLIFY_CHAR(str[13 + 2]) ), \
	(X_UNHEXLIFY_CHAR(str[14 + 2]) << 4) | (X_UNHEXLIFY_CHAR(str[15 + 2]) ), \
	(X_UNHEXLIFY_CHAR(str[16 + 3]) << 4) | (X_UNHEXLIFY_CHAR(str[17 + 3]) ), \
	(X_UNHEXLIFY_CHAR(str[18 + 3]) << 4) | (X_UNHEXLIFY_CHAR(str[19 + 3]) ), \
	(X_UNHEXLIFY_CHAR(str[20 + 4]) << 4) | (X_UNHEXLIFY_CHAR(str[21 + 4]) ), \
	(X_UNHEXLIFY_CHAR(str[22 + 4]) << 4) | (X_UNHEXLIFY_CHAR(str[23 + 4]) ), \
	(X_UNHEXLIFY_CHAR(str[24 + 4]) << 4) | (X_UNHEXLIFY_CHAR(str[25 + 4]) ), \
	(X_UNHEXLIFY_CHAR(str[26 + 4]) << 4) | (X_UNHEXLIFY_CHAR(str[27 + 4]) ), \
	(X_UNHEXLIFY_CHAR(str[28 + 4]) << 4) | (X_UNHEXLIFY_CHAR(str[29 + 4]) ), \
	(X_UNHEXLIFY_CHAR(str[30 + 4]) << 4) | (X_UNHEXLIFY_CHAR(str[31 + 4]) ), \
}}

struct x_uuid_st {
	uint8_t u8[16];
};

int x_uuid_gen_v1(x_uuid *uuid);
int x_uuid_gen_v4(x_uuid *uuid);
int x_uuid_gen_v7(x_uuid *uuid);
void x_uuid_text(const x_uuid *uuid, char *out);
int x_uuid_parse(const char *text, x_uuid *out);

X_EXTERN_C_END

#endif
