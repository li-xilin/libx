/*
 * Copyright (c) 2020-2025 Borislav Stanimirov
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
#ifndef X_DEBUG_H
#define X_DEBUG_H

#include "linkage.h"
#include "types.h"
#include "detect.h"
#include "macros.h"

X_EXTERN_C_BEGIN

#define X_TRACE_MAX 512

struct x_location_st {
	const char *file;
	const char *func;
	int line;
};

#ifdef __cplusplus
#define X_WHERE ([](const char *fn, int line) -> const x_location* { \
	static const x_location loc = {  __FILE__, fn, line }; \
	return &loc; }(__func__, __LINE__))
#else
#define X_WHERE ((const x_location[1]) { { __FILE__, __func__, __LINE__ } })
#endif

struct x_trace_st
{
	void *frame[X_TRACE_MAX];
	size_t depth;
};

void x_trace_back(x_trace *trace);
int x_trace_depth(x_trace *trace);
void x_trace_dump(const x_trace *trace, x_strbuf *sb);
#define x_trace_fputs(trace, fp) do { \
	x_strbuf _sb; \
	x_strbuf_init(&_sb); \
	x_trace_dump(trace, &_sb); \
	x_fputs(x_strbuf_str(&_sb), fp); \
	x_strbuf_free(&_sb); \
} while (0)

void __x_dump_memory(const char *name, const void *p, size_t size, x_strbuf *sb);
#define x_hexdump(p, size, sb) __x_dump_memory(#p, p, size, sb);
#define x_hexdump_fp(p, size, fp) do { \
	x_strbuf _sb; \
	x_strbuf_init(&_sb); \
	x_memory_dump(p, size, &_sb); \
	x_fputs(x_strbuf_str(&_sb), fp); \
	x_strbuf_free(&_sb); \
} while (0)

#define x_reach(fp) x_fprintf(fp, x_u("reach %s at %s:%d (version %s %s)\n"), __func__, __FILE__, __LINE__, __DATE__, __TIME__)

void __x_sanitize_stub(const x_location *loc);

#define x_sanitize_stub() __x_sanitize_stub(X_WHERE)

X_EXTERN_C_END

#endif
