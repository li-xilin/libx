/*
 * Copyright (c) 2024 Li Xilin <lixilin@gmx.com>
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

#include "x/log.h"
#include "x/detect.h"
#include "x/uchar.h"
#include "x/tcolor.h"
#include "x/mutex.h"
#include "x/file.h"
#include "x/macros.h"
#include "x/printf.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include <limits.h>

static int s_mode = 0;
static x_log_handler_fn *s_handler = NULL;
static void *s_handler_arg = NULL;
static x_mutex s_lock = X_MUTEX_INIT;

int x_log_mode(void)
{
	return s_mode;
}

void x_log_set_mode(int mode)
{
	s_mode = mode;
}

void x_log_set_handler(x_log_handler_fn *f, void *arg)
{
	s_handler = f;
	s_handler_arg = arg;
}

static inline const char *__basename(const char *path)
{
	int c = 0, i;
	for (i = 0; path[i]; i++)
#ifdef X_OS_WIN
		if (path[i] == '\\')
#else
			if (path[i] == '/')
#endif
				c = i + 1;
	return path + c;
}

int x_log_handler_native(const x_location *loc, void *arg, int level, const x_uchar* text)
{
	int retval = -1;
	char time_buf[64];
	char loc_buf[512];
	char level_buf[32];
	x_uchar buffer[1024];
	const char *type;
	int mode = x_log_mode();
	FILE *fp = arg ? arg : stderr;
	x_tcolor tc = { 0 };

	time_buf[0] = '\0';
	loc_buf[0] = '\0';

	if (!(mode & X_LM_NOTIME)) {
		time_t tim = time(NULL);
		struct tm *t = localtime(&tim);
		snprintf(time_buf, sizeof time_buf, "%4d-%02d-%02d %02d:%02d:%02d",
				t->tm_year + 1900, t->tm_mon + 1,
				t->tm_mday, t->tm_hour, t->tm_min, t->tm_sec);
	}

	if (!(mode & X_LM_NOLOC)) {
		snprintf(loc_buf, sizeof loc_buf, "%s:%s:%d", __basename(loc->file), loc->func, loc->line);
	}

	x_mutex_lock(&s_lock);

	if (x_fputc(x_u('['), fp) < 0)
		goto out;

	x_tcolor_set(fp, &tc);
	switch(level) {
		case X_LL_TRACE:
			type = "TRACE";
			x_tcolor_fg(&tc, X_TCOLOR_BBLUE);
			break;
		case X_LL_DEBUG:
			type = "DEBUG";
			x_tcolor_fg(&tc, X_TCOLOR_BLUE);
			break;
		case X_LL_INFO:
			type = "INFO";
			x_tcolor_fg(&tc, X_TCOLOR_GREEN);
			break;
		case X_LL_WARN:
			type = "WARN";
			x_tcolor_fg(&tc, X_TCOLOR_YELLOW);
			break;
		case X_LL_ERROR:
			type = "ERROR";
			x_tcolor_fg(&tc, X_TCOLOR_RED);
			break;
		case X_LL_FATAL:
			type = "FATAL";
			x_tcolor_fg(&tc, X_TCOLOR_BRED);
			break;
		default:
			snprintf(level_buf, sizeof level_buf, "%d", level);
			type = level_buf;
			x_tcolor_fg(&tc, X_TCOLOR_GREEN);
	}
	if (x_ansi_to_ustr(type, buffer, sizeof buffer) == -1) {
		x_tcolor_reset(&tc);
		goto out;
	}
	if (x_fprintf(fp, x_u("%-5S"), buffer) < 0) {
		x_tcolor_reset(&tc);
		goto out;
	}
	x_tcolor_reset(&tc);
	if (x_fputc(x_u(']'), fp) < 0)
		goto out;

	if (time_buf[0]) {
		if (x_fputc(x_u('['), fp) < 0)
			goto out;
		if (x_ansi_to_ustr(time_buf, buffer, sizeof buffer) == -1)
			goto out;
		x_tcolor_set(fp, &tc);
		x_tcolor_fg(&tc, X_TCOLOR_GREEN);
		if (x_fprintf(fp, x_u("%S"), buffer) < 0) {
			x_tcolor_reset(&tc);
			goto out;
		}
		x_tcolor_reset(&tc);
		if (x_fputc(x_u(']'), fp) < 0) {
			goto out;
		}
	}

	if (loc_buf[0]) {
		if (x_fputc(x_u('['), fp) < 0)
			goto out;
		if (x_ansi_to_ustr(loc_buf, buffer, sizeof buffer) == -1)
			goto out;
		x_tcolor_set(fp, &tc);
		x_tcolor_fg(&tc, X_TCOLOR_GREEN);
		if (x_fprintf(fp, x_u("%S"), buffer) < 0) {
			x_tcolor_reset(&tc);
			goto out;
		}
		x_tcolor_reset(&tc);
		if (x_fputc(x_u(']'), fp) < 0) {
			goto out;
		}
	}

	if (x_fputc(x_u(' '), fp) == EOF)
		goto out;

	if (x_fputs(text, fp) == EOF)
		goto out;

#ifdef X_OS_WIN
	if (x_fputc(x_u('\r'), fp) == EOF)
		goto out;
#endif

	if (x_fputc(x_u('\n'), fp) == EOF)
		goto out;

	if (fflush(fp))
		goto out;
	retval = 0;
out:
	x_mutex_unlock(&s_lock);
	return retval;
}

int x_log_handler_utf8(const x_location *loc, void *arg, int level, const x_uchar* text)
{
	int retval = -1;
	char time_buf[64];
	char loc_buf[512];
	char level_buf[32];
	char utf8_buf[1024];
	const char *type;
	int mode = x_log_mode();
	FILE *fp = arg ? arg : stderr;
	x_tcolor tc = { 0 };

	time_buf[0] = '\0';
	loc_buf[0] = '\0';

	if (!(mode & X_LM_NOTIME)) {
		time_t tim = time(NULL);
		struct tm *t = localtime(&tim);
		snprintf(time_buf, sizeof time_buf, "%4d-%02d-%02d %02d:%02d:%02d",
				t->tm_year + 1900, t->tm_mon + 1,
				t->tm_mday, t->tm_hour, t->tm_min, t->tm_sec);
	}

	if (!(mode & X_LM_NOLOC)) {
		snprintf(loc_buf, sizeof loc_buf, "%s:%s:%d", __basename(loc->file), loc->func, loc->line);
	}

	x_mutex_lock(&s_lock);

	if (fputc('[', fp) < 0)
		goto out;

	x_tcolor_set(fp, &tc);
	switch(level) {
		case X_LL_TRACE:
			type = "TRACE";
			x_tcolor_fg(&tc, X_TCOLOR_BBLUE);
			break;
		case X_LL_DEBUG:
			type = "DEBUG";
			x_tcolor_fg(&tc, X_TCOLOR_BLUE);
			break;
		case X_LL_INFO:
			type = "INFO";
			x_tcolor_fg(&tc, X_TCOLOR_GREEN);
			break;
		case X_LL_WARN:
			type = "WARN";
			x_tcolor_fg(&tc, X_TCOLOR_YELLOW);
			break;
		case X_LL_ERROR:
			type = "ERROR";
			x_tcolor_fg(&tc, X_TCOLOR_RED);
			break;
		case X_LL_FATAL:
			type = "FATAL";
			x_tcolor_fg(&tc, X_TCOLOR_BRED);
			break;
		default:
			snprintf(level_buf, sizeof level_buf, "%d", level);
			type = level_buf;
			x_tcolor_fg(&tc, X_TCOLOR_GREEN);
	}
	if (fprintf(fp, "%-5s", type) < 0) {
		x_tcolor_reset(&tc);
		goto out;
	}
	x_tcolor_reset(&tc);
	if (fputs("]", fp) < 0)
		goto out;

	if (time_buf[0]) {
		if (fputc('[', fp) < 0)
			goto out;
		x_tcolor_set(fp, &tc);
		x_tcolor_fg(&tc, X_TCOLOR_GREEN);
		if (fprintf(fp, "%s", time_buf) < 0) {
			x_tcolor_reset(&tc);
			goto out;
		}
		x_tcolor_reset(&tc);
		if (fputc(']', fp) < 0) {
			goto out;
		}
	}

	if (loc_buf[0]) {
		if (fputc('[', fp) < 0)
			goto out;
		x_tcolor_set(fp, &tc);
		x_tcolor_fg(&tc, X_TCOLOR_GREEN);
		// FIXME: __FILE__ or __func__ are sometimes not utf-8
		if (fprintf(fp, "%s", loc_buf) < 0) {
			x_tcolor_reset(&tc);
			goto out;
		}
		x_tcolor_reset(&tc);
		if (fputc(']', fp) < 0) {
			goto out;
		}
	}

	if (fputc(' ', fp) == EOF)
		goto out;

	if (x_ustr_to_utf8(text, utf8_buf, sizeof utf8_buf) == -1)
		goto out;
	if (fputs(utf8_buf, fp) == EOF)
		goto out;

	if (!utf8_buf[0] || text[strlen(utf8_buf) - 1] != '\n')
		if (fputc('\n', fp) == EOF)
			goto out;

	retval = 0;
out:

	if (fflush(fp))
		goto out;
	x_mutex_unlock(&s_lock);
	return retval;
}

int __x_log_print(const x_location *loc, int level, const x_uchar* fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	int retval = __x_log_vprint(loc, level, fmt, ap);
	va_end(ap);
	return retval;
}

int __x_log_vprint(const x_location *loc, int level, const x_uchar* fmt, va_list ap)
{
	int mode = x_log_mode();
	switch(level)
	{
		default:
		case X_LL_TRACE:
			if (mode & X_LM_NOTRACE)
				return 0;
			break;
		case X_LL_DEBUG:
			if (mode & X_LM_NODEBUG)
				return 0;
			break;
		case X_LL_INFO:
			if (mode & X_LM_NOINFO)
				return 0;
			break;
		case X_LL_WARN:
			if (mode & X_LM_NOWARN)
				return 0;
			break;
		case X_LL_ERROR:
			if (mode & X_LM_NOERROR)
				return 0;
			break;
		case X_LL_FATAL:
			if (mode & X_LM_NOFATAL)
				return 0;
			break;
	}

	x_uchar ms_buf[X_LOG_MX];
	if (x_vsnprintf(ms_buf, sizeof ms_buf, fmt, ap) < 0)
		return -1;

	for (int len = x_ustrlen(ms_buf); len; len--) {
		if (ms_buf[len - 1] == x_u('\n') || ms_buf[len - 1] == x_u('\r') || ms_buf[len - 1] == x_u('\t')
				|| ms_buf[len - 1] == x_u(' ')) {
			ms_buf[len - 1] = x_u('\0');
			continue;
		}
		else
			break;
	}

	int ret = s_handler ? s_handler(loc, s_handler_arg, level, ms_buf)
		: x_log_handler_native(loc, s_handler_arg, level, ms_buf);
	return ret;
}

