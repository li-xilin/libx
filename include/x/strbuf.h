/**
 * Copyright (c) 2017-2020 Steve Bennett <steveb@workware.net.au>
 * Copyright (c) 2023,2025 Li Xilin <lixilin@gmx.com>
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


#ifndef X_STRBUF_H
#define X_STRBUF_H

#include "types.h"
#include "uchar.h"

#define X_STRBUF_INIT { 0 }

struct x_strbuf_st
{
	int remaining;
	int last;
	int chars;
	x_uchar *line;
	x_uchar *data;
};

void x_strbuf_init(x_strbuf *sb);
void x_strbuf_free(x_strbuf *sb);
x_strbuf *x_strbuf_copy(x_strbuf *sb);
int x_strbuf_len(x_strbuf *sb);
int x_strbuf_chars(x_strbuf *sb);
void x_strbuf_append(x_strbuf *sb, const x_uchar *str);
void x_strbuf_append_len(x_strbuf *sb, const x_uchar *str, int len);
void x_strbuf_insert(x_strbuf *sb, int index, const x_uchar *str);
/**
 * If len is -1, deletes to the end of the buffer.
 */
void x_strbuf_delete(x_strbuf *sb, int index, int len);
void x_strbuf_clear(x_strbuf *sb);
x_uchar *x_strbuf_str(const x_strbuf *sb);
x_uchar *x_strbuf_data(x_strbuf *sb);
x_uchar *x_strbuf_gets(x_strbuf *sb);
void x_strbuf_endgets(x_strbuf *sb);

#endif
