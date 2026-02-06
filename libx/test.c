/*
 * Copyright (c) 2021,2025 Li Xilin <lixilin@gmx.com>
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

#include "x/test.h"
#include "x/string.h"
#include "x/splay.h"
#include "x/memory.h"
#include "x/uchar.h"
#include <setjmp.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>
#include <inttypes.h>
#include <errno.h>

struct notice
{
	x_link link;
	const char *file;
	int line;
	char text[];
};

void ut_case_free(ut_case *tc)
{
	while (!x_list_is_empty(&tc->msg_list)) {
		struct notice *m = x_container_of(x_list_first(&tc->msg_list), struct notice, link);
		x_list_del(&m->link);
		free(m);
	}
	free(tc->name);
	free(tc->file);
	free(tc->log);
}

int ut_case_init(ut_case *tc, const char *name, int line, ut_check_fn *fn)
{
	memset(tc, 0, sizeof *tc);
	if (!(tc->name = x_strdup(name)))
		return -1;
	x_list_init(&tc->msg_list);
	tc->line = line;
	tc->state = UT_CS_READY;
	tc->proc = fn;
	return 0;
}

int ut_case_copy(ut_case *dst_tc, const ut_case *src_tc)
{
	memcpy(dst_tc, src_tc, sizeof *dst_tc);
	dst_tc->name = dst_tc->log = dst_tc->file = NULL;
	dst_tc->name = x_strdup(src_tc->name);
	if (!src_tc->name) {
		errno = EINVAL;
		goto fail;
	}

	if (src_tc->log)
		if (!(dst_tc->log =  x_strdup(src_tc->log)))
			goto fail;
	if (src_tc->file)
		if (!(dst_tc->file =  x_strdup(src_tc->file)))
			goto fail;
	x_list_init(&dst_tc->msg_list);

	return 0;
fail:
	free(dst_tc->name);
	free(dst_tc->log);
	free(dst_tc->file);
	return -1;
}

int ut_case_add_text(ut_case *c, const char *file, int line, char *text)
{
	int size = strlen(text) + 1;
	struct notice *m = malloc(sizeof *m + size);
	if (!m)
		return -1;
	memcpy(m->text, text, size);
	m->file = file;
	m->line = line;
	x_list_add_back(&c->msg_list, &m->link);
	return 0;
}

void ut_runner_free(ut_runner *r)
{
	if (!r)
		return;
	x_list_popeach(pos, &r->suites) {
		// ut_suite *s = x_container_of(pos, ut_suite, link);
		// free(s->name);
		// free(s);
	}
}

#define TITLE_FMT "testting %-10s :"

void ut_case_dump_rope(const char *suite_name, ut_case *tc, void *arg)
{
	x_rope *out = arg;
	assert(tc->state != UT_CS_READY);
	x_list_foreach(link, &tc->msg_list) {
		struct notice *m = x_container_of(link, struct notice, link);
		x_rope_printf(out, x_rope_length(out),"[INFO] " TITLE_FMT " %s:%s:%d: %s\n",
				suite_name, tc->name, m->file, m->line, m->text);
	}
	switch (tc->state) {
		case UT_CS_PASS:
			x_rope_printf(out, x_rope_length(out), "[ OK ] " TITLE_FMT " %s\n", suite_name, tc->name);
			break;
		case UT_CS_FAIL:
			x_rope_printf(out, x_rope_length(out), "[FAIL] " TITLE_FMT " %s:%s:%d: %s\n",
				suite_name, tc->name, tc->file, tc->line, tc->log ? tc->log : "none");
			break;
		case UT_CS_TERM:
			x_rope_printf(out, x_rope_length(out), "[TERM] " TITLE_FMT " %s:%s:%d: %s\n",
				suite_name, tc->name, tc->file, tc->line, tc->log ? tc->log : "none");
			break;
	}
}

void ut_case_dump_file(const char *suite_name, ut_case *tc, void *arg)
{
	FILE *fp = arg;
	assert(tc->state != UT_CS_READY);
	x_list_foreach(link, &tc->msg_list) {
		struct notice *m = x_container_of(link, struct notice, link);
		fprintf(fp, "[INFO] " TITLE_FMT " %s:%s:%d: %s\n",
				suite_name, tc->name, m->file, m->line, m->text);
	}
	switch (tc->state) {
		case UT_CS_PASS:
			fprintf(fp, "[ OK ] " TITLE_FMT " %s\n", suite_name, tc->name);
			break;
		case UT_CS_FAIL:
			fprintf(fp, "[FAIL] " TITLE_FMT " %s:%s:%d: %s\n",
				suite_name, tc->name, tc->file, tc->line, tc->log ? tc->log : "none");
			break;
		case UT_CS_TERM:
			fprintf(stderr, "[TERM] " TITLE_FMT " %s:%s:%d: %s\n",
				suite_name, tc->name, tc->file, tc->line, tc->log ? tc->log : "none");
			break;
	}
}

int ut_runner_summary(const ut_runner *r, int *pass, int *term)
{
	if (pass)
		*pass = r->pass;
	if (term)
		*term = r->term;
	return r->fail;
}

void ut_runner_add(ut_runner *r, ut_suite* s)
{
	assert(r != NULL);
	assert(s != NULL);
	x_list_add_back(&r->suites, &s->link);
}

void ut_runner_remove(ut_runner *r, ut_suite* s)
{
	assert(r != NULL);
	assert(s != NULL);
	x_list_del(&s->link);
}

void ut_runner_run(ut_runner *r, ut_process_f *process_cb)
{
	int case_tested = 0, case_pass = 0;
	ut_output_fn *output_cb = r->output_cb ? r->output_cb : ut_case_dump_rope;
	r->pass = 0;
	r->fail = 0;
	r->term = 0;
	int case_total = 0;
	x_list_foreach(pos, &r->suites) {
		ut_suite *s = x_container_of(pos, ut_suite, link);
		case_total += s->suite_cnt;
	}
	x_list_foreach(pos, &r->suites) {
		const ut_suite *s = x_container_of(pos, ut_suite, link);
		r->current.arg = s->arg;

		x_list_foreach(pos1, &s->tctab) {
			ut_case *c = x_container_of(pos1, ut_case, link);
			jmp_buf jmp;
			if (c->state != UT_CS_READY)
				continue;
			if (process_cb)
				process_cb(s->name, c->name, case_tested + 1, case_total);
			r->current.jump_ptr = &jmp;
			r->current.tc = c;
			int jmpid = setjmp(jmp);
			switch (jmpid) {
				case 0:
					c->proc(r);
					c->state = UT_CS_PASS;
					case_pass ++;
					break;
				case 1:
					c->state = UT_CS_FAIL;
					break;
				case 2:
					c->state = UT_CS_TERM;
					break;
			}
			output_cb(s->name, c, r->output_arg);
			case_tested ++;
		}

	}
	r->current.arg = NULL;
}

void *ut_runner_arg(const ut_runner *r)
{
	return r->current.arg;
}

static void leave(ut_runner *r, ut_case_state cs, const char *file, int line, const char *fmt, va_list args)
{
	free(r->current.tc->file);
	r->current.tc->file = x_strdup(file);

	r->current.tc->line = line;

	free(r->current.tc->log);
	r->current.tc->log = NULL;
	if (fmt) {
		char buf[1024];
		vsprintf(buf, fmt, args);
		r->current.tc->log =  x_strdup(buf);
	}

	assert(cs == UT_CS_FAIL || cs == UT_CS_TERM);
	if (cs == UT_CS_FAIL)
		longjmp(*r->current.jump_ptr, 1);
	else
		longjmp(*r->current.jump_ptr, 2);
}

void __ut_assert(ut_runner *r, bool cond, const char *file, int line, const char *fmt, ...)
{
	if (cond)
		return;
	va_list args;
	va_start(args, fmt);
	leave(r, UT_CS_FAIL, file, line, fmt, args);
	va_end(args);
}

void __ut_printf(ut_runner *r, const char *file, int line, const char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	char buf[1024];
	strcpy(buf, "failed to print log message.");
	vsnprintf(buf, sizeof buf, fmt, ap);
	ut_case_add_text(r->current.tc, file, line, buf);
	va_end(ap);
}

void __ut_assert_str_equal(ut_runner *r, const char *ex, const char *ac, const char *file, int line)
{
	if (strcmp(ex, ac) == 0)
		return;
	__ut_fail(r, file, line, "test failed: expect '%s', but actual '%s'", ex, ac);
}

void __ut_assert_ustr_equal(ut_runner *r, const x_uchar *ex, const x_uchar *ac, const char *file, int line)
{
	if (x_ustrcmp(ex, ac) == 0)
		return;
	char ex_ansi[1024], ac_ansi[1024];
	x_ustr_to_ansi(ex, ex_ansi, sizeof ex_ansi);
	x_ustr_to_ansi(ac, ac_ansi, sizeof ac_ansi);
	__ut_fail(r, file, line, "test failed: expect '%s', but actual '%s'", ex_ansi, ac_ansi);
}

void __ut_assert_mem_equal(ut_runner *r, const void *ex, size_t exsize, const void *ac, size_t acsize, const char *file, int line)
{
	int index = -1;
	int cmp = 0;
	const char *exp = ex, *acp = ac;
	if (exsize != acsize) {
		__ut_fail(r, file, line, "test failed: expect size is %zd, but actual %zd", exsize, acsize);
		return;
	}
	for (int i = 0; i < exsize; i++) {
		if (exp[i] != acp[i]) {
			cmp = exp[i] - acp[i];
			index = i;
			break;
		}
	}
	if (cmp == 0)
		return;
	char ex_buf[66 + 1];
	char ac_buf[66 + 1];
	size_t left = index - x_min(index, 16);
	size_t right = index + x_min(exsize - (index + 1), 16 + 1);
	
	x_memtohex(exp + left, index - left, ex_buf);
	sprintf(ex_buf + (index - left) * 2, "[%hhX]", exp[index]);
	x_memtohex(exp + index + 1, right - index, ex_buf + (index - left) * 2 + 4);

	x_memtohex(acp + left, index - left, ac_buf);
	sprintf(ac_buf + (index - left) * 2, "[%hhX]", acp[index]);
	x_memtohex(acp + index + 1, right - index, ac_buf + (index - left) * 2 + 4);

	__ut_fail(r, file, line, "test failed: at offset +%d\n"
			"\texpect: '%s'\n"
			"\tactual: '%s'", index, ex_buf, ac_buf);
}

void __ut_assert_int_equal(ut_runner *r, int64_t ex, int64_t ac, const char *file, int line)
{
	if (ex == ac)
		return;
	__ut_fail(r, file, line, "test failed: expect '%" PRId64 "', but actual '%" PRId64 "'", ex, ac);
}

void __ut_assert_uint_equal(ut_runner *r, uint64_t ex, uint64_t ac, const char *file, int line)
{
	if (ex == ac)
		return;
	__ut_fail(r, file, line, "test failed: expect '%" PRIu64 "', but actual '%" PRIu64 "'", ex, ac);
}

void __ut_fail(ut_runner *r, const char *file, int line, const char *fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	leave(r, UT_CS_FAIL, file, line, fmt, args);
	va_end(args);
}

void __ut_term(ut_runner *r, const char *file, int line, const char *fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	leave(r, UT_CS_TERM, file, line, fmt, args);
	va_end(args);
}

void ut_runner_init(ut_runner *r, ut_output_fn *output_cb, void *arg)
{
	memset(r, 0, sizeof *r);
	r->output_cb = output_cb;
	r->output_arg = arg;
	x_list_init(&r->suites);
}

void ut_suite_free(ut_suite *s)
{
	if (!s)
		return;
	x_list_popeach(cur, &s->tctab) {
		ut_case *c = x_container_of(cur, ut_case, link);
		ut_case_free(c);
		free(c);
	}
}

void ut_suite_init(ut_suite *s, const char* name)
{
	assert(name != NULL);
	assert(s != NULL);
	assert(strlen(name) < 256);
	x_list_init(&s->tctab);
	strcpy(s->name, name);
}

int ut_suite_add_case(ut_suite *suite, const char *name, ut_check_fn *proc)
{
	ut_case *c = malloc(sizeof *c);
	if (!c)
		return -1;
	ut_case_init(c, name, 0, proc);
	x_list_add_back(&suite->tctab, &c->link);
	suite->suite_cnt++;
	return 0;
}
