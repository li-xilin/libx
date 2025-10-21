/*
 * Copyright (c) 2017-2020 Steve Bennett <steveb@workware.net.au>
 * Copyright (c) 2023,2025 Li Xilin <lixilin@gmx.com>
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

#include "x/strbuf.h"
#include "x/unicode.h"
#include "x/string.h"
#include "x/memory.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include <assert.h>

#define SB_INCREMENT 4096

void x_strbuf_init(x_strbuf *sb)
{
	sb->remaining = 0;
	sb->last = 0;
	sb->chars = 0;
	sb->data = NULL;
}

void x_strbuf_free(x_strbuf *sb)
{
	x_free(sb->data);
}

static void strbuf_realloc(x_strbuf *sb, int newlen)
{
	sb->data = (x_uchar *)x_realloc(sb->data, newlen * sizeof(x_uchar));
	sb->remaining = newlen - sb->last;
}

void x_strbuf_append(x_strbuf *sb, const x_uchar *str)
{
	x_strbuf_append_len(sb, str, x_ustrlen(str));
}

void x_strbuf_append_len(x_strbuf *sb, const x_uchar *str, int len)
{
	if (sb->remaining < len + 1)
		strbuf_realloc(sb, sb->last + len + 1 + SB_INCREMENT);
	memcpy(sb->data + sb->last, str, len * sizeof(x_uchar));
	sb->data[sb->last + len] = 0;
	sb->last += len;
	sb->remaining -= len;
	sb->chars += x_ustr_charcnt(str, len);
}

x_uchar *x_strbuf_data(x_strbuf *sb)
{
	if (!sb->data)
		sb->data = x_zalloc(NULL, sizeof(x_uchar));
	x_uchar *data = sb->data;
	x_strbuf_init(sb);
	return data;
}

/* Moves up all the data at position 'pos' and beyond by 'len' bytes
 * to make room for new data
 *
 * Note: Does *not* update sb->chars
 */
static void strbuf_insert_space(x_strbuf *sb, int pos, int len)
{
	assert(pos <= sb->last);

	/* Make sure there is enough space */
	if (sb->remaining < len) {
		strbuf_realloc(sb, sb->last + len + SB_INCREMENT);
	}
	/* Now move it up */
	memmove(sb->data + pos + len, sb->data + pos, (sb->last - pos) * sizeof(x_uchar));
	sb->last += len;
	sb->remaining -= len;
	/* And null terminate */
	sb->data[sb->last] = 0;
}

/**
 * Move down all the data from pos + len, effectively
 * deleting the data at position 'pos' of length 'len'
 */
static void strbuf_delete_space(x_strbuf *sb, int pos, int len)
{
	assert(pos < sb->last);
	assert(pos + len <= sb->last);
	sb->chars -= x_ustr_charcnt(sb->data + pos, len);
	/* Now move it up */
	memmove(sb->data + pos, sb->data + pos + len, (sb->last - pos - len) * sizeof(x_uchar));
	sb->last -= len;
	sb->remaining += len;
	/* And null terminate */
	sb->data[sb->last] = 0;
}

void x_strbuf_insert(x_strbuf *sb, int index, const x_uchar *str)
{
	if (index >= sb->last) {
		/* Inserting after the end of the list appends. */
		x_strbuf_append(sb, str);
	}
	else {
		int len = x_ustrlen(str);

		strbuf_insert_space(sb, index, len);
		memcpy(sb->data + index, str, len * sizeof(x_uchar));
		sb->chars += x_ustr_charcnt(str, len);
	}
}

/**
 * Delete the bytes at index 'index' for length 'len'
 * Has no effect if the index is past the end of the list.
 */
void x_strbuf_delete(x_strbuf *sb, int index, int len)
{
	if (index < sb->last) {
		x_uchar *pos = sb->data + index;
		if (len < 0)
			len = sb->last;
		strbuf_delete_space(sb, pos - sb->data, len);
	}
}

void x_strbuf_clear(x_strbuf *sb)
{
	if (sb->data) {
		/* Null terminate */
		sb->data[0] = 0;
		sb->last = 0;
		sb->chars = 0;
	}
}

int x_strbuf_len(x_strbuf *sb)
{
	return sb->last;
}

int x_strbuf_chars(x_strbuf *sb)
{
	return sb->chars;
}

x_uchar *x_strbuf_str(const x_strbuf *sb)
{
	if (!sb->data)
		return x_u("");
	return sb->data;
}
