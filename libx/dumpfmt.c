/*
 * Copyright (c) 2021-2024,2025 Li Xilin <lixilin@gmx.com>
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

#include "x/dump.h"
#include "x/string.h"
#include "x/detect.h"
#include "x/printf.h"
#include "x/uchar.h"
#include <stdint.h>
#include <string.h>
#include <wchar.h>
#include <assert.h>
#include <inttypes.h>

static int default_snumber(int64_t value, x_dump_out_cb_f *print_fn, void *ctx)
{
	x_uchar buf[24];
	int ret = x_sprintf(buf, x_u("%") PRIi64, value);
	if ((ret = print_fn(buf, ret, ctx)))
		return ret;
	return 0;
}

static int default_unumber(uint64_t value, x_dump_out_cb_f *print_fn, void *ctx)
{
	x_uchar buf[24];
	int ret = x_sprintf(buf, x_u("%") PRIu64, value);
	if ((ret = print_fn(buf, ret, ctx)))
		return ret;
	return 0;
}

static int default_fnumber(double value, x_dump_out_cb_f *print_fn, void *ctx)
{
	x_uchar buf[24];
	int ret = x_sprintf(buf, x_u("%lg"), value);
	if ((ret = print_fn(buf, ret, ctx)))
		return ret;
	return 0;
}

static int default_pointer(const void *value, x_dump_out_cb_f *print_fn, void *ctx)
{
	x_uchar buf[24];
	int ret = x_sprintf(buf, x_u("%p"), value);
	if ((ret = print_fn(buf, ret, ctx)))
		return ret;
	return 0;
}

static int default_string(const x_uchar *value, size_t length, x_dump_out_cb_f *print_fn, void *ctx)
{
	x_uchar buf[1024];
	int ret = x_sprintf(buf, x_u("%p \""), (void *)value);
	if ((ret = print_fn(buf, ret, ctx)))
		return ret;

	if ((ret = print_fn(value, length, ctx)))
		return ret;
	x_ustrcpy(buf, x_u("\""));
	if ((ret = print_fn(buf, 1, ctx)))
		return ret;

	return 0;
}

static int default_memory(const uint8_t *value, size_t size, x_dump_out_cb_f *print_fn, void *ctx)
{
	x_uchar out_buf[256];
	int ret = x_sprintf(out_buf, x_u("%p \\"), (void *)value);
	if ((ret = print_fn(out_buf, ret, ctx)))
		return ret;
	size_t shift = 0;
	while (shift != size) {
		char buf[256];
		size_t partsiz = (size - shift) % (sizeof(buf) / 2 - 1);
		x_memtohex(value + shift, partsiz, buf);
		x_utf8_to_ustr(buf, out_buf, x_arrlen(out_buf));
		if ((ret = print_fn(out_buf, partsiz * 2, ctx)))
			return ret;
		shift += partsiz;
	}
	return 0;
}

static int default_symbol(const x_uchar *name, x_dump_out_cb_f *print_fn, void *ctx)
{
	return print_fn(name, x_ustrlen(name), ctx);
}

static int default_pair_left(x_dump_out_cb_f *print_fn, void *ctx)
{
	return 0;
}

static int default_pair_midst(x_dump_out_cb_f *print_fn, void *ctx)
{
	return print_fn(x_u(" = "), 3, ctx);
}

static int default_pair_right(x_dump_out_cb_f *print_fn, void *ctx)
{
	return 0;
}

static int default_block_left(const x_uchar *label, x_dump_out_cb_f *print_fn, void *ctx)
{
	if (label) {
		if (print_fn(label, x_ustrlen(label), ctx))
			return -1;
		if (print_fn(x_u(" "), 1, ctx))
			return -1;
	}
	return print_fn(x_u("{"), 1, ctx);
}

static int default_block_midst(size_t index, x_dump_out_cb_f *print_fn, void *ctx)
{
	if (index == 0)
		return 0;
	return print_fn(x_u(", "), 2, ctx);
}

static int default_block_right(const x_uchar *label, x_dump_out_cb_f *print_fn, void *ctx)
{
	return print_fn(x_u("}"), 1, ctx);
}

static int default_nomem(x_dump_out_cb_f *print_fn, void *ctx)
{
	x_uchar buf[] = x_u("!ENOMEM");
	return print_fn(buf, x_arrlen(buf) - 1, ctx);
}

static int default_indent(int depth, x_dump_out_cb_f *print_fn, void *ctx)
{
	return 0;
}

static const struct x_dump_format_st default_format = 
{
	.snumber = default_snumber,
	.unumber = default_unumber,
	.fnumber = default_fnumber,
	.pointer = default_pointer,
	.string = default_string,
	.memory = default_memory,
	.symbol = default_symbol,
	.pair_left = default_pair_left,
	.pair_midst = default_pair_midst,
	.pair_right = default_pair_right,
	.block_left = default_block_left,
	.block_midst = default_block_midst,
	.block_right = default_block_right,
	.nomem = default_nomem,
	.indent = default_indent,
};

static int pretty_snumber(int64_t value, x_dump_out_cb_f *print_fn, void *ctx)
{
	x_uchar buf[64];
	int ret = x_sprintf(buf, x_u("%") PRIi64, value);
	if ((ret = print_fn(buf, ret, ctx)))
		return ret;
	return 0;
}

static int pretty_unumber(uint64_t value, x_dump_out_cb_f *print_fn, void *ctx)
{
	x_uchar buf[64];
	int ret = x_sprintf(buf, x_u("%") PRIu64, value);
	if ((ret = print_fn(buf, ret, ctx)))
		return ret;
	return 0;
}

static int pretty_fnumber(double value, x_dump_out_cb_f *print_fn, void *ctx)
{
	x_uchar buf[64];
	int ret = x_sprintf(buf, x_u("%lg"), value);
	if ((ret = print_fn(buf, ret, ctx)))
		return ret;
	return 0;
}

static int pretty_pointer(const void *value, x_dump_out_cb_f *print_fn, void *ctx)
{
	x_uchar buf[64];
	int ret = x_sprintf(buf, x_u("%p"), value);
	if ((ret = print_fn(buf, ret, ctx)))
		return ret;
	return 0;
}

static int pretty_string(const x_uchar *value, size_t length, x_dump_out_cb_f *print_fn, void *ctx)
{
	int ret;
	if ((ret = print_fn(x_u("\""), 1, ctx)))
		return ret;

	if ((ret = print_fn(value, length, ctx)))
		return ret;

	if ((ret = print_fn(x_u("\""), 1, ctx)))
		return ret;

	return 0;
}

static int pretty_memory(const uint8_t *value, size_t size, x_dump_out_cb_f *print_fn, void *ctx)
{
	int ret;
	if ((ret = print_fn(x_u("\\"), 1, ctx)))
		return ret;
	size_t shift = 0;
	while (shift != size) {
		char buf[256];
		x_uchar out_buf[256];
		size_t partsiz = (size - shift) % (sizeof(buf) / 2 - 1);
		x_memtohex(value + shift, partsiz, buf);
		x_utf8_to_ustr(buf, out_buf, x_arrlen(out_buf));
		if ((ret = print_fn(out_buf, partsiz * 2, ctx)))
			return ret;
		shift += partsiz;
	}
	return 0;
}

static int pretty_symbol(const x_uchar *name, x_dump_out_cb_f *print_fn, void *ctx)
{
	x_uchar buf[1024];
	int ret = x_sprintf(buf, x_u("%s"), name);
	return print_fn(buf, ret, ctx);
}

static int pretty_pair_left(x_dump_out_cb_f *print_fn, void *ctx)
{
	return 0;
}

static int pretty_pair_midst(x_dump_out_cb_f *print_fn, void *ctx)
{
	x_uchar equal[] = x_u(" = ");
	return print_fn(equal, x_arrlen(equal) - 1, ctx);
}

static int pretty_pair_right(x_dump_out_cb_f *print_fn, void *ctx)
{
	return 0;
}

static int pretty_block_left(const x_uchar *label, x_dump_out_cb_f *print_fn, void *ctx)
{
	if (label) {
		if (print_fn(label, x_ustrlen(label), ctx))
			return -1;
	}
	if (print_fn(x_u(" {\n"), 3, ctx))
		return -1;
	return 0;
}

static int pretty_block_midst(size_t index, x_dump_out_cb_f *print_fn, void *ctx)
{
	if (index == 0)
		return 0;
	return print_fn(x_u(",\n"), 2, ctx);
}

static int pretty_block_right(const x_uchar *label, x_dump_out_cb_f *print_fn, void *ctx)
{
	return print_fn(x_u("\n}"), 2, ctx);
}

static int pretty_nomem(x_dump_out_cb_f *print_fn, void *ctx)
{
	x_uchar buf[] = x_u("!ENOMEM");
	return print_fn(buf, x_arrlen(buf) - 1, ctx);
}

static int pretty_indent(int depth, x_dump_out_cb_f *print_fn, void *ctx)
{
	x_uchar buf[1024];
	for (int i = 0; i < depth * 2; i++) {
		buf[i] = x_u(' ');
	}
	return print_fn(buf, depth * 2, ctx);
}

static const struct x_dump_format_st pretty_format =
{
	.snumber = pretty_snumber,
	.unumber = pretty_unumber,
	.fnumber = pretty_fnumber,
	.pointer = pretty_pointer,
	.string = pretty_string,
	.memory = pretty_memory,
	.symbol = pretty_symbol,
	.pair_left = pretty_pair_left,
	.pair_midst = pretty_pair_midst,
	.pair_right = pretty_pair_right,
	.block_left = pretty_block_left,
	.block_midst = pretty_block_midst,
	.block_right = pretty_block_right,
	.nomem = pretty_nomem,
	.indent = pretty_indent,
};

const x_dump_format *x_dump_default_format()
{
	return &default_format;
}

const x_dump_format *x_dump_pretty_format()
{
	return &pretty_format;
}

