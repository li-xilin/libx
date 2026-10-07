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

#include "types.h"
#include "linkage.h"

X_EXTERN_C_BEGIN

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
