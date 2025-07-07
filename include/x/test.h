/*
 * Copyright (c) 2021,2024 Li Xilin <lixilin@gmx.com>
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

#ifndef X_TEST_H
#define X_TEST_H

#include "x/types.h"
#include "x/rope.h"
#include "x/list.h"
#include <setjmp.h>

#ifndef UT_RUNNER_DEFINED
#define UT_RUNNER_DEFINED
typedef struct ut_runner_st ut_runner;
#endif

#ifndef UT_SUITE_DEFINED
#define UT_SUITE_DEFINED
typedef struct ut_suite_st ut_suite;
#endif

#ifndef UT_CASE_DEFINED
#define UT_CASE_DEFINED
typedef struct ut_case_st ut_case;
#endif

#define UT_NAME_MAX 128
#define UT_LOG_MAX 128

typedef void ut_output_fn(const char* suite_name, ut_case *tc, void *arg);
typedef void ut_check_fn(ut_runner *runner);
typedef void ut_process_f(const char *suite_name, const char *case_name, int pos, int total);

struct ut_runner_st
{
	ut_output_fn *output_cb;
	void *output_arg;
	x_list suites;
	int pass, fail, term;
	struct {
		char *name;
		jmp_buf *jump_ptr;
		ut_case *tc;
		void *arg;
	} current;
};

struct ut_suite_st
{
	x_link link;
	char name[256];
	void *arg;
	size_t suite_cnt;
	x_list tctab;
};


typedef enum ut_case_state_en
{
        UT_CS_READY = 0,
        UT_CS_PASS,
        UT_CS_FAIL,
        UT_CS_TERM,
} ut_case_state;

struct ut_case_st
{
	x_link link;
	ut_check_fn *proc;
	char *name;
	char *log;
	char *file;
	unsigned int line;
	int state;
	x_list msg_list;
};

typedef void ax_case_enum_text_f(ut_case *c, const char *file, int line, char *msg, void *ctx);

void ut_case_free(ut_case *tc);

int ut_case_copy(ut_case *dst_tc, const ut_case *src_tc);

int ut_case_add_text(ut_case *c, const char *file, int line, char *text);

void ut_case_enum_text(ut_case *c, ax_case_enum_text_f *f, void *ctx);

int ut_case_init(ut_case *tc, const char *name, int line, ut_check_fn *fn);

void ut_case_dump_rope(const char *suite_name, ut_case *tc, void *arg);

void ut_case_dump_file(const char *suite_name, ut_case *tc, void *arg);

void ut_suite_init(ut_suite *s, const char* name);

void ut_suite_free(ut_suite *s);

int ut_suite_add_case(ut_suite *s, const char *name, ut_check_fn *proc);

#define ut_suite_add(suite, proc) ut_suite_add_case((suite), #proc, (proc))

void ut_runner_free(ut_runner *r);

void ut_runner_init(ut_runner *r, ut_output_fn *output_cb, void *arg);

const char *ut_runner_result(const ut_runner *r);

int ut_runner_summary(const ut_runner *r, int *pass, int *term);

void ut_runner_add(ut_runner *r, ut_suite* s);

void ut_runner_remove(ut_runner *r, ut_suite* s);

void ut_runner_run(ut_runner *r, ut_process_f *f);

void *ut_runner_arg(const ut_runner *r);

void __ut_assert(ut_runner *r, bool cond, const char *file, int line, const char *fmt, ...);

void __ut_assert_str_equal(ut_runner *r, const char *ex, const char *ac, const char *file, int line);

void __ut_assert_mem_equal(ut_runner *r, const void *ex, size_t exlen, const void *ac, size_t aclen, const char *file, int line);

void __ut_assert_int_equal(ut_runner *r, int64_t ex, int64_t ac, const char *file, int line);

void __ut_assert_uint_equal(ut_runner *r, uint64_t ex, uint64_t ac, const char *file, int line);

void __ut_fail(ut_runner *r, const char *file, int line, const char *fmt, ...);

void __ut_term(ut_runner *r, const char *file, int line, const char *fmt, ...);

void __ut_printf(ut_runner *r, const char *file, int line, const char *fmt, ...);

#define ut_assert(r, cond) __ut_assert((r), (cond), __FILE__, __LINE__, "assertion failed: %s", #cond)

#define ut_printf(r, ...) __ut_printf((r), __FILE__, __LINE__, __VA_ARGS__)

#define ut_assert_msg(r, cond, ...) __ut_assert((r), (cond), __FILE__, __LINE__, __VA_ARGS__)

#define ut_fail(r, ...) __ut_fail((r), __FILE__, __LINE__, __VA_ARGS__)

#define ut_term(r, ...) __ut_term((r), __FILE__, __LINE__, __VA_ARGS__)

#define ut_assert_str_equal(r, ex, ac) __ut_assert_str_equal((r), ex, ac, __FILE__, __LINE__)

#define ut_assert_mem_equal(r, ex, exlen, ac, aclen) __ut_assert_mem_equal((r), (ex), (exlen), (ac), (aclen), __FILE__, __LINE__)

#define ut_assert_int_equal(r, ex, ac) __ut_assert_int_equal((r), ex, ac, __FILE__, __LINE__)

#define ut_assert_uint_equal(r, ex, ac) __ut_assert_uint_equal((r), ex, ac, __FILE__, __LINE__)

#endif

