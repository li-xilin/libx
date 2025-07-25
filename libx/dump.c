/*
 * Copyright (c) 2021-2023 Li Xilin <lixilin@gmx.com>
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
#include "x/assert.h"
#include "x/uchar.h"
#include "x/file.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <wchar.h>
#include <assert.h>
#include <stdbool.h>
#include <inttypes.h>
#include <ctype.h>

#ifndef NDEBUG
#define check_symbol(_sym) \
do { \
	assert(_sym); \
	x_assert(_sym[0] != '\0', \
			"invalid zero-length symbol name"); \
	x_assert(isalpha((int)_sym[0]) || _sym[0] == '_', \
			"symbol name %s begin with invalid charactor", _sym); \
	for (int i = 1; _sym[i]; i++) { \
		x_assert(_sym[i] == '_' || _sym[i] == '.' || isdigit((int)_sym[i]) || isalpha((int)_sym[i]), \
				"invalid charactor \'%c\' in symbol name \'%s\'", _sym[i], _sym); \
	} \
} while(0)
#else
#define check_symbol(_sym) (void)0
#endif

enum dump_type {
	DTYPE_SNUM,
	DTYPE_UNUM,
	DTYPE_FNUM,
	DTYPE_PTR,
	DTYPE_STR,
	DTYPE_MEM,
	DTYPE_SYM,
	DTYPE_PAIR,
	DTYPE_BLOCK,
	DTYPE_NOMEM,
	DTYPE_BIND = 0x10,
};

struct search_args
{
	const x_dump_format *format;
	x_dump_out_cb_f *out_cb;
	x_dump_out_cb_f *filter_cb;
	int depth;
	void *ctx;
};

union value_u
{
	intmax_t snum;
	uintmax_t unum;
	double fnum;
	const void *ptr;

	struct value_mem_st {
		const void *maddr;
		size_t size;
		x_uchar data[];
	} mem, str, wcs, sym;

	struct value_pair_st {
		x_dump *first;
		x_dump *second;
	} pair;

	struct value_block_st {
		size_t len;
		x_uchar *name;
		x_dump *dumps[];
	} block;
};

struct x_dump_st
{
	enum dump_type type;
	x_uchar value[];
};

struct x_dump_st g_dmp_nomem = { .type = DTYPE_NOMEM };
#define NOMEM_DMP (&g_dmp_nomem)

inline static void set_bind_bit(x_dump *dmp, bool set);
static void dump_rec_free(x_dump *dmp);
static int dump_out_dfs(const x_dump *dmp, int depth, struct search_args *args);

#ifndef NDEBUG
inline static bool bind_bit(const x_dump *dmp, bool nomem_bit)
{
	return dmp->type == DTYPE_NOMEM ? nomem_bit : (dmp->type & DTYPE_BIND);
}
#endif

inline static void set_bind_bit(x_dump *dmp, bool set)
{
	if (dmp->type != DTYPE_NOMEM)
		dmp->type = set ? (dmp->type | DTYPE_BIND)
			: (dmp->type & ~DTYPE_BIND);
}

x_dump *x_dump_int(intmax_t val)
{
	x_dump *dmp = malloc(sizeof(x_dump) + sizeof val);
	if (!dmp)
		return NOMEM_DMP;
	dmp->type = DTYPE_SNUM;
	*(intmax_t *)dmp->value = val;
	return dmp;
}

x_dump *x_dump_uint(uintmax_t val)
{
	x_dump *dmp = malloc(sizeof(x_dump) + sizeof val);
	if (!dmp)
		return NOMEM_DMP;
	dmp->type = DTYPE_UNUM;
	*(uintmax_t *)dmp->value = val;
	return dmp;
}

x_dump *x_dump_float(double val)
{
	x_dump *dmp = malloc(sizeof(x_dump) + sizeof val);
	if (!dmp)
		return NOMEM_DMP;
	dmp->type = DTYPE_FNUM;
	*(double *)dmp->value = val;
	return dmp;
}

x_dump *x_dump_ptr(const void *val)
{
	x_dump *dmp = malloc(sizeof(x_dump) + sizeof val);
	if (!dmp)
		return NOMEM_DMP;
	dmp->type = DTYPE_PTR;
	*(const void **)dmp->value = val;
	return dmp;

}

x_dump *x_dump_str(const x_uchar *val)
{
	if (val == NULL)
		return x_dump_symbol(x_u("NULL"));
	size_t len = x_ustrlen(val);
	size_t siz = (len + 1) * sizeof(x_uchar);
	x_dump *dmp = malloc(sizeof(x_dump) + sizeof(struct value_mem_st) + siz);
	if (!dmp)
		return NOMEM_DMP;

	dmp->type = DTYPE_STR;
	union value_u *value = (void *)dmp->value;
	value->str.size = len;
	value->str.maddr = val;
	memcpy(value->str.data, val, siz);

	return dmp;
}

x_dump *x_dump_mem(const void *ptr, size_t size)
{
	if (ptr == NULL)
		return x_dump_symbol(x_u("NULL"));

	x_dump *dmp = malloc(sizeof(x_dump) + sizeof(struct value_mem_st) + size);
	if (!dmp)
		return NOMEM_DMP;

	dmp->type = DTYPE_MEM;
	union value_u *value = (void *)dmp->value;
	value->mem.size = size;
	value->str.maddr = ptr;
	memcpy(value->mem.data, ptr, size);

	return dmp;
}


x_dump *x_dump_symbol(const x_uchar *sym)
{
	check_symbol(sym);

	size_t len = x_ustrlen(sym);
	size_t siz = (len + 1) * sizeof(x_uchar);
	x_dump *dmp = malloc(sizeof(x_dump) + sizeof(struct value_mem_st) + siz);
	if (!dmp)
		return NOMEM_DMP;

	dmp->type = DTYPE_SYM;
	union value_u *value = (void *)dmp->value;
	value->sym.size = len;
	memcpy(value->sym.data, sym, siz);

	return dmp;
}

x_dump *x_dump_pair(x_dump *d1, x_dump *d2)
{
	x_assert(!bind_bit(d1, 0), "The first dump already bound with other one");
	x_assert(!bind_bit(d2, 0), "The second dump already bound with other one");

	x_dump *dmp = malloc(sizeof(x_dump) + sizeof(struct value_pair_st));
	if (!dmp) {
		set_bind_bit(d1, 1);
		set_bind_bit(d2, 1);
		dump_rec_free(d1);
		dump_rec_free(d2);
		return NOMEM_DMP;
	}

	dmp->type = DTYPE_PAIR;
	struct value_pair_st *pair = (void *)dmp->value;
	set_bind_bit(d1, 1);
	set_bind_bit(d2, 1);
	pair->first = d1;
	pair->second = d2;
	return dmp;
}

x_dump *x_dump_empty_block(const x_uchar *sym, size_t len)
{
	check_symbol(sym);

	x_dump *dmp = calloc(1, sizeof(x_dump) + sizeof(struct value_block_st) + len * sizeof (x_dump *) );
	if (!dmp)
		return NOMEM_DMP;

	dmp->type = DTYPE_BLOCK;
	struct value_block_st *block = (void *) dmp->value;
	block->len = len;
	block->name = x_ustrdup(sym);
	if (!block->name ) {
		free(dmp);
		return NOMEM_DMP;
	}

	for (size_t i = 0; i < len; i++)
		block->dumps[i] = NULL;

	return dmp;
}

int x_dump_set_name(x_dump *dmp, const x_uchar *sym)
{
	check_symbol(sym);
	x_assert(dmp->type == DTYPE_BLOCK, "unsupported dump type");
	struct value_block_st *block = (void *) dmp->value;
	free(block->name);
	block->name = x_ustrdup(sym);
	if (!block->name) {
		free(dmp);
		return true;
	}
	return false;
}

void x_dump_bind(x_dump *dmp, int index, x_dump* binding)
{
	assert(dmp);
	assert(index >= 0);
	assert(binding);
	x_assert(!bind_bit(dmp, 0), "binding operation is denied");
	x_assert(dmp->type == DTYPE_BLOCK || dmp->type == DTYPE_NOMEM,
			"binding operation only support block and nomem"); 
	x_assert(!bind_bit(binding, 0), "instance is already in binding");
	struct value_block_st *block;
#ifndef NDEBUG
	if (binding->type == DTYPE_BLOCK) {
		block = (void *) binding->value;
		for (int i = 0; i < block->len; i++) {
			x_assert(block->dumps[i], "incomplete dump of block, NULL pointer field #%d", i);
		}
	}
#endif
	block = (void *) dmp->value;
	switch (dmp->type) {
		case DTYPE_BLOCK:
			x_assert(index >= 0 && index < block->len, "dump: exceed block field index %d", index);
			x_assert(!block->dumps[index], "field #%d already set", index);
			block->dumps[index] = binding;
			set_bind_bit(binding, 1);
			break;
		case DTYPE_NOMEM:
			set_bind_bit(binding, 1);
			dump_rec_free(binding);
			break;
		default:
			break;
	}
}

x_dump *x_dump_block(const x_uchar *sym, ...)
{
	size_t n = 0;
	va_list ap;
	va_start(ap, sym);
	while (va_arg(ap, void *))
		n++;
	va_end(ap);
	x_dump *blk = x_dump_empty_block(sym, n);
	va_start(ap, sym);
	for (int i = 0; i < n; i++) {
		x_dump *v = va_arg(ap, void *);
		x_dump_bind(blk, i, v);
	}
	va_end(ap);
	return blk;
}

static void dump_rec_free(x_dump *dmp)
{
	assert(bind_bit(dmp, 1));
	union {
		struct value_block_st *block;
		struct value_pair_st *pair;
	} val;
	switch (dmp->type & ~DTYPE_BIND) {
		case DTYPE_SNUM:
		case DTYPE_UNUM:
		case DTYPE_FNUM:
		case DTYPE_PTR:
		case DTYPE_STR:
		case DTYPE_MEM:
		case DTYPE_SYM:
			break;
		case DTYPE_PAIR:
			val.pair = (void *) dmp->value;
			dump_rec_free(val.pair->first);
			dump_rec_free(val.pair->second);
			break;
		case DTYPE_BLOCK:
			val.block = (void *) dmp->value;
			for (size_t i = 0; i < val.block->len; i++) {
				dump_rec_free(val.block->dumps[i]);
			}
			free(val.block->name);
			break;
		case DTYPE_NOMEM:
			return;
		default:
			abort();
	}
	free(dmp);
}

void x_dump_free(x_dump *dmp)
{
	assert(dmp);
	x_assert(!bind_bit(dmp, 0),
			"failed to free dump, the instance has beed bound");

	set_bind_bit(dmp, 1);
	dump_rec_free(dmp);
}

static int write_file_cb(const x_uchar *buf, size_t len, void *ctx)
{
	FILE *fp = ctx;
	return x_fprintf(fp, x_u("%.*s"), (int)len, buf);
}

static int indent_check_cb(const x_uchar *buf, size_t len, void *ctx)
{
	struct search_args *args = ctx;
	for (size_t i = 0; i < len; i++)
		x_assert(buf[i] != '\n' && buf[i] != '\0', "");
	return args->out_cb(buf, len, args->ctx);
}

static int filter_cb(const x_uchar *buf, size_t len, void *ctx)
{

	struct search_args *args = ctx;
	size_t start = 0;
	while (start != len) {
		size_t end;
		for (end = start; end < len; end++) {
			switch (buf[end]) {
			case '\n':
				if (args->out_cb(buf + start, end - start + 1, args->ctx))
					return -1;
				args->format->indent(args->depth, indent_check_cb, args);
				end += 1;
				goto end_for;
			case '\0':
				if (args->out_cb(buf + start, end - start, args->ctx))
					return -1;
				if (args->out_cb(x_u("\\0"), 2, args->ctx))
					return -1;
				end += 1;
				goto end_for;
			}
		}
		if (args->out_cb(buf + start, end - start, args->ctx))
			return -1;
end_for:
		start = end;
	}
	return 0;
}

int x_dump_fput(const x_dump *dmp, const x_dump_format *format, FILE *fp)
{
	int fail = x_dump_serialize(dmp, format, write_file_cb, fp) ? true : false;
	fputc('\n', fp);
	fflush(fp);
	return fail;
}

static int dump_out_dfs(const x_dump *dmp, int depth, struct search_args *args)
{
	int ret;
	if (!dmp) {
		if (args->format->symbol(x_u("NOT_BIND"), args->filter_cb, args))
			return -1;
		return 0;
	}
	union value_u *value = (void *)dmp->value;
	switch (dmp->type & ~DTYPE_BIND) {
		case DTYPE_SNUM:
			if (args->format->snumber(value->snum, args->filter_cb, args))
				return -1;
			break;
		case DTYPE_UNUM:
			if (args->format->unumber(value->unum, args->filter_cb, args))
				return -1;
			break;
		case DTYPE_FNUM:
			if (args->format->fnumber(value->fnum, args->filter_cb, args))
				return -1;
			break;
		case DTYPE_PTR:
			if (args->format->pointer(value->ptr, args->filter_cb, args))
				return -1;
			break;
		case DTYPE_STR:
			if (args->format->string(value->str.maddr, value->str.size, args->filter_cb, args))
				return -1;
			break;
		case DTYPE_MEM:
			if (args->format->memory(value->mem.maddr, value->mem.size, args->filter_cb, args))
				return -1;
			break;
		case DTYPE_SYM:
			if (args->format->symbol(value->sym.data, args->filter_cb, args))
				return -1;
			break;
		case DTYPE_NOMEM:
			if (args->format->nomem(args->filter_cb, args))
				return -1;
			break;
		case DTYPE_PAIR:
			if (args->format->pair_left(args->filter_cb, args))
				return -1;
			if (dump_out_dfs(value->pair.first, depth + 1, args))
				return -1;
			if (args->format->pair_midst(args->filter_cb, args))
				return -1;
			if (dump_out_dfs(value->pair.second, depth + 1, args))
				return -1;
			if (args->format->pair_right(args->filter_cb, args))
				return -1;
			break;
		case DTYPE_BLOCK:
			if (args->format->indent) {
				args->depth++;
			}
			ret = args->format->block_left(value->block.name, args->filter_cb, args);
			if (ret < 0)
				return -1;
				
			for (size_t i = 0; i < value->block.len; i++) {
				if (args->format->block_midst(i, args->filter_cb, args))
					return -1;
				if (dump_out_dfs(value->block.dumps[i], depth, args))
					return -1;
			}
			if (args->format->indent)
				args->depth--;
			if (args->format->block_right(value->block.name, args->filter_cb, args))
				return -1;
			break;
		default:
			abort();
	}
	return 0;
}

int x_dump_serialize(const x_dump *dmp, const x_dump_format *format, x_dump_out_cb_f *cb, void *ctx)
{
	assert(dmp);
	assert(cb);

	struct search_args args = {
		.filter_cb = filter_cb,
		.format = format ? format : x_dump_default_format(),
		.out_cb = cb,
		.depth = 0,
		.ctx = ctx,
	};
	return dump_out_dfs(dmp, 0, &args);
}

