/*
 * Copyright (c) 2023-2024 Li Xilin <lixilin@gmx.com>
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include "x/ini.h"
#include "x/list.h"
#include "x/assert.h"
#include "x/string.h"

#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <ctype.h>

#define BASE_LEN 1024
#define MAX_LEN (BASE_LEN * 1024 * 1024) /* aligned with BASE_LEN, here is 1GB */ 
#define DEFAULT_SEC_NAME "__default"
#if defined(X_OS_WIN)
#define COMMENT_CHAR ';'
#else
#define COMMENT_CHAR '#'
#endif

static void free_section(struct x_ini_section_st *sec);
static void free_option(struct x_ini_option_st *opt);

x_ini *x_ini_create(const char *ext_keych)
{
	if (ext_keych && strlen(ext_keych) > 14) {
		errno = ENAMETOOLONG;
		return NULL;
	}
	x_ini *d = malloc(sizeof *d);
	if (!d)
		return NULL;

	if (ext_keych)
		strcpy(d->allowed_ch, ext_keych);
	else
		d->allowed_ch[0] = '\0';
	d->size = 0 ;
	x_list_init(&d->sec_list);
	return d ;
}

void x_ini_free(x_ini * d)
{
	if (!d)
		return;

	while (!x_list_is_empty(&d->sec_list)) {
		struct x_ini_section_st *sec = x_container_of(x_list_first(&d->sec_list), struct x_ini_section_st, link);
		x_list_del(&sec->link);
		while (!x_list_is_empty(&sec->opt_list)) {
			struct x_ini_option_st *opt = x_container_of(x_list_first(&sec->opt_list), struct x_ini_option_st, link);
			x_list_del(&opt->link);
			free_option(opt);
		}
		free_section(sec);
	}
	free(d);
}

static struct x_ini_option_st *find_option(struct x_ini_section_st *sec, const char *key)
{
	size_t key_hash = x_strihash(key);
	x_list_foreach(cur_opt, &sec->opt_list) {
		struct x_ini_option_st *opt = x_container_of(cur_opt, struct x_ini_option_st, link);
		if (!opt->key)
			continue;
		if (key_hash != opt->hash)
			continue;
		if (x_stricmp(key, opt->key))
			continue;
		return opt;
	}
	return NULL;
}

static struct x_ini_section_st *find_section(const x_ini *d, const char *sec_name)
{
	if (!sec_name || sec_name[0] == '\0')
		sec_name = DEFAULT_SEC_NAME;

	size_t sec_hash = x_strihash(sec_name);

	x_list_foreach(cur_sec, &d->sec_list) {
		struct x_ini_section_st *sec = x_container_of(cur_sec, struct x_ini_section_st, link);
		if (sec_hash != sec->hash)
			continue;
		if (x_stricmp(sec_name, sec->name))
			continue;
		return sec;
	}
	return NULL;
}

static struct x_ini_section_st *find_section_with_len(const x_ini *d, const char *sec_name, size_t sec_name_len)
{
	if (!sec_name || sec_name_len == 0) {
		sec_name = DEFAULT_SEC_NAME;
		sec_name_len = strlen(sec_name);
	}

	size_t sec_hash = x_strnihash(sec_name, sec_name_len);

	x_list_foreach(cur_sec, &d->sec_list) {
		struct x_ini_section_st *sec = x_container_of(cur_sec, struct x_ini_section_st, link);
		if (sec_hash != sec->hash)
			continue;
		if (x_strnicmp(sec_name, sec->name, sec_name_len))
			continue;
		return sec;
	}
	return NULL;
}

const char *x_ini_get(const x_ini *d, const char *sec_name, const char *key)
{
	struct x_ini_section_st *sec = find_section(d, sec_name);
	if (!sec)
		return NULL;
	struct x_ini_option_st *opt = find_option(sec, key);
	if (!opt)
		return NULL;
	return opt->val;
}

static void free_section(struct x_ini_section_st *sec)
{
	if (!sec)
		return;
	free(sec->name);
	free(sec->comment);
	free(sec);
}

static void free_option(struct x_ini_option_st *opt)
{
	if (opt) {
		free(opt->key);
		free(opt->val);
		free(opt->comment);
		free(opt);
	}
}

static struct x_ini_section_st *alloc_section(const char *sec_name, const char *comment)
{
	struct x_ini_section_st *sec = NULL;

	if (!(sec = calloc(1, sizeof *sec)))
		goto fail;
	sec->hash = x_strihash(sec_name);
	if (!(sec->name = x_strdup(sec_name)))
		goto fail;
	
	if (comment && !(sec->comment = x_strdup(comment)))
		goto fail;
	x_list_init(&sec->opt_list);
	x_link_init(&sec->link);
	return sec;
fail:
	free_section(sec);
	return NULL;
}

static struct x_ini_option_st *alloc_option(const char *key, const char *val, const char *comment)
{
	struct x_ini_option_st *opt = NULL;

	if (!(opt = calloc(1, sizeof *opt)))
		goto fail;
	if (key) {
		opt->hash = x_strihash(key);
		if (!(opt->key = x_strdup(key)))
			goto fail;
		if (!(opt->val = x_strdup(val)))
			goto fail;
	}
	if (comment && !(opt->comment = x_strdup(comment)))
		goto fail;
	x_link_init(&opt->link);
	return opt;
fail:
	free_option(opt);
	return NULL;
}

int x_ini_set(x_ini *d, const char *sec_name, const char *key, const char *val, const char *comment)
{
	int retval = -1;
	struct x_ini_option_st *opt = NULL;
	struct x_ini_section_st *sec = NULL;

	x_assert(key && val, "NULL pointer for sec_name, key or val");

	if (sec_name && !x_ini_check_section_name(sec_name)) {
		errno = EINVAL;
		return -1;
	}

	if (!x_ini_check_key_name(key, d->allowed_ch)) {
		errno = EINVAL;
		return -1;
	}

	if (!(sec = find_section(d, sec_name))) {
		if (!(sec = alloc_section(sec_name, NULL)))
			goto out;
		if (!(opt = alloc_option(key, val, comment))) {
			free_section(sec);
			goto out;
		}
		x_list_add_back(&d->sec_list, &sec->link);
		x_list_add_back(&sec->opt_list, &opt->link);
		d->size++;
	}
	else if (!(opt = find_option(sec, key))){
		opt = alloc_option(key, val, comment);
		if (!sec)
			goto out;
		x_list_add_back(&sec->opt_list, &opt->link);
		d->size++;
	}
	else {
		char *val_dup = x_strdup(val);
		if (!val_dup)
			goto out;

		free(opt->val);
		opt->val = val_dup;
		if (comment) {
			free(opt->comment);
			opt->comment = x_strdup(comment);
		}
	}
	retval = 0;
out:
	return retval;
}

int x_ini_push_sec(x_ini *d, const char *sec_name, const char *comment)
{
	int retval = -1;
	struct x_ini_section_st *sec = NULL;
	if ((sec = find_section(d, sec_name))) {
		errno = EEXIST;
		goto out;
	}
	if (!(sec = alloc_section(sec_name, comment)))
		goto out;
	x_list_add_back(&d->sec_list, &sec->link);
	retval = 0;
out:
	return retval;
}

int x_ini_push_opt(x_ini *d, const char *key, const char *val, const char *comment)
{
	int retval = -1;
	struct x_ini_option_st *opt = NULL;
	struct x_ini_section_st *sec = NULL;
	if (x_list_is_empty(&d->sec_list)) {
		if (!(sec = alloc_section(DEFAULT_SEC_NAME, NULL)))
			goto out;
		x_list_add_back(&d->sec_list, &sec->link);
	}

	sec = x_container_of(x_list_last(&d->sec_list), struct x_ini_section_st, link);
	if (key) {
		if ((opt = find_option(sec, key))){
			errno = EEXIST;
			return -1;
		}
	}

	if (!(opt = alloc_option(key, val, comment)))
		goto out;

	x_list_add_back(&sec->opt_list, &opt->link);
	d->size++;
	retval = 0;
out:
	return retval;
}

void x_ini_unset(x_ini *d, const char *sec_name, const char *key)
{
	struct x_ini_section_st *sec = find_section(d, sec_name);
	if (!sec)
		return;
	struct x_ini_option_st *opt = find_option(sec, key);
	if (!opt)
		return;

	x_list_del(&opt->link);
	d->size--;

	if (!x_list_is_empty(&sec->opt_list))
		return;

	x_list_del(&sec->link);
	free_section(sec);
	return ;
}

static void print_comment(FILE *out, const char *comment)
{
	fputc(COMMENT_CHAR, out);
	if (comment[0] != ' '
			&& comment[0] != '\t'
			&& comment[0] != COMMENT_CHAR
			&& comment[0] != '\0')
		fputc(' ', out);
	fputs(comment, out);
}

void x_ini_dump(const x_ini *d, FILE *out)
{
	// x_link *cur_sec, *cur_opt;
	x_list_foreach(cur_sec, &d->sec_list) {
		struct x_ini_section_st *sec = x_container_of(cur_sec, struct x_ini_section_st, link);
		if (&sec->link != d->sec_list.head.next || x_stricmp(sec->name, DEFAULT_SEC_NAME) != 0) {
			fprintf(out, "[%s]", sec->name);
			if (sec->comment && sec->comment[0]) {
				fputc(' ', out);
				print_comment(out, sec->comment);
			}
			fputc('\n', out);
		}
		x_list_foreach(cur_opt, &sec->opt_list) {
			struct x_ini_option_st *opt = x_container_of(cur_opt, struct x_ini_option_st, link);
			if (opt->key)
				fprintf(out, "%s = %s", opt->key, opt->val);
			if (opt->comment) {
				if (!opt->key)
					print_comment(out, opt->comment);
				else if (opt->comment[0]) {
					fputc(' ', out);
					print_comment(out, opt->comment);
				}
			}
			fputc('\n', out);
		}
	}
}

static char *strstrip(char *s)
{
	char *last = NULL ;
	last = s + strlen(s);
	while (isspace((int)*s) && *s)
		s++;
	while (last > s) {
		if (!isspace((int)*(last-1)))
			break ;
		last--;
	}
	*last = '\0';
	return s;
}

bool x_ini_check_section_name(const char *name)
{
	if (name[0] == '\0')
		return false;
	if (!isalpha(name[0]) && !isdigit(name[0]) && name[0] != '_')
		return false;
	for (int i = 1; name[i]; i++) {
		if (!isalpha(name[i]) && !isdigit(name[i]) && name[i] != '_' && name[i] != '.' && name[i] != '-' && name[i] != ' ')
			return false;
	}
	return true;
}

bool x_ini_check_key_name(const char *name, const char *ext_chars)
{
	int l = -1, r = -1;
	if (name[0] == '\0')
		return false;
	if (!isalpha(name[0]) && !isdigit(name[0]) && name[0] != '_')
		return false;
	for (int i = 1; name[i]; i++) {
		if (name[i] == '[') {
			if (l > 0)
				return false;
			l = i;
			continue;
		}
		if (name[i] == ']') {
			if (r > 0)
				return false;
			r = i;
			continue;
		}
		if (l > 0 && r < 0)
			continue;
		if (isalpha(name[i]) || isdigit(name[i]) || name[i] == '_')
			continue;
		bool found = false;
		for (int j = 0; ext_chars[j]; j++) {
			if (name[i] == ext_chars[j]) {
				found = true;
				break;
			}
		}
		if (found)
			continue;
		return false;
	}
	if (l > 0 || r > 0) {
		if (l <= 0 || r <= 0)
			return false;
		if (name[r + 1] != '\0')
			return false;
		if (l + 1 >= r)
			return false;
	}
	return true;
}

static int parse_line(char *line_buf, x_ini *d)
{
	char *val = NULL, *comment = NULL;
	char *line = strstrip(line_buf);

	/* split string by specify token char */
	for (int i = 0; line[i]; i++) {
		if (line[i] == ';' || line[i] == '#') {
			line[i] = '\0';
			comment = line + i + 1;
			break;
		}

		if (!val && line[i] == '=') {
			line[i] = '\0';
			val = line + i + 1;
		}
	}

	if (!val) {
		char *name = strstrip(line);
		if (name[0] == 0) {
			if (x_ini_push_opt(d, NULL, NULL, comment))
				return -1;
			return 0;
		}
		if (name[0] != '[')
			return X_INI_ESYNTAX;

		size_t len = strlen(line);
		if (len == 0 || name[len - 1] != ']')
			return X_INI_EBADNAME;

		name[len - 1] = '\0';
		char *sec_name = strstrip(name + 1);
		if (!x_ini_check_section_name(sec_name))
			return X_INI_EBADNAME;

		if (x_ini_push_sec(d, sec_name, comment))
			return -1;
	}
	else {
		char *key = strstrip(line);
		if (!x_ini_check_key_name(key, d->allowed_ch))
			return X_INI_EBADNAME;

		char *val1 = strstrip(val);
		if (x_ini_push_opt(d, key, val1, comment))
			return -1;
	}
	return 0;
}

x_ini *x_ini_load(FILE *fp, const char *ext_keych, x_ini_parse_error_f *error_cb, void *args)
{
	x_ini *d = NULL;
	char *line_buf = NULL;
	size_t buf_offset = 0, buf_len = BASE_LEN, lineno = 1;
	char line[BASE_LEN];

	if (!(line_buf = malloc(sizeof(char) * buf_len * 2)))
		goto fail;

	if (!(d = x_ini_create(ext_keych)))
		goto fail;

	while (fgets(line, sizeof line, fp)) {
		int line_len = strlen(line);
		if (line_len == 0)
			continue;

		memcpy(line_buf + buf_offset, line, line_len + 1);
		buf_offset += line_len;

		if (line_buf[buf_offset - 1] != '\n') {
			/* incomplete text for a line */
			if (buf_len - buf_offset >= sizeof line)
				continue;

			buf_len *= 2;

			if (buf_len >= MAX_LEN) {
				/* max size limit exceed*/
				int ch;
				if (error_cb && error_cb(lineno, X_INI_ETOOLONG, args))
					goto out;
				/* consume the remaining chars in current line */
				while (((ch = fgetc(fp)) != '\n' && ch != EOF));
				buf_offset = 0;
			}
			else {
				char *new_buf = realloc(line_buf, buf_len * sizeof(char));
				if (!new_buf)
					goto fail;
				line_buf = new_buf;
			}
		}
		else {
			line_buf[buf_offset - 1] = '\0';
			buf_offset = 0;

			int err = parse_line(line_buf, d);
			if (err == -1)
				goto fail;
			if (err > 0 && error_cb && error_cb(lineno, err, args))
				goto out;
			lineno++;
		}
	}
	if (!feof(fp)) {
		errno = EIO;
		goto fail;
	}
	goto out;
fail:
	x_ini_free(d);
	d = NULL;
out:
	free(line_buf);
	return d;
}

const char *x_ini_strerror(int errcode)
{
	switch (errcode) {
		case X_INI_EBADNAME: return "Invalid key name";
		case X_INI_ETOOLONG: return "Too long for single line";
		case X_INI_ESYNTAX: return "Syntax error";
		default: return "";
	}
}

static int section_name_len(const char *path)
{
	for (int i = 0; path[i]; i++)
		if (path[i] == ':')
			return i;
	return -1;
}

char *x_ini_path_vget(const x_ini *d, const char *path_fmt, va_list ap)
{
	char path[256];
	path[sizeof path - 1] = '\0';
	vsnprintf(path, sizeof path - 1, path_fmt, ap);
	int sec_len = section_name_len(path);
	x_assert(sec_len >= 0, "no colon(:) found in field path");
	struct x_ini_section_st *sec = find_section_with_len(d, path, sec_len);
	if (!sec)
		return NULL;
	struct x_ini_option_st *opt = find_option(sec, path + sec_len + 1);
	if (!opt)
		return NULL;
	return opt->val;
}

char *x_ini_path_get(const x_ini *d, const char *path_fmt, ...)
{
	va_list ap;
	va_start(ap, path_fmt);
	char *ret = x_ini_path_vget(d, path_fmt, ap);
	va_end(ap);
	return ret;
}

int x_ini_path_set(x_ini *d, const char *path, const char *fmt, ...)
{
	int ret = -1;
	char buf[1024], *sec_name = NULL, *val = NULL;
	va_list ap;
	int sec_len = section_name_len(path);
	x_assert(sec_len >= 0, "no colon(:) found in field path");

	sec_name = malloc(sec_len + 1);
	if (!sec_name)
		goto out;
	memcpy(sec_name, path, sec_len);
	sec_name[sec_len] = '\0';

	va_start(ap, fmt);
	size_t len = vsnprintf(buf, sizeof buf, fmt, ap);
	if (len != strlen(buf)) {
		val = malloc(len + 1);
		if (!val)
			goto out;
		vsnprintf(val, sizeof buf, fmt, ap);
	}

	ret = x_ini_set(d, sec_name, path + sec_len + 1, val ? val : buf, NULL);
out:
	va_end(ap);
	free(val);
	free(sec_name);
	return ret;
}

int x_ini_get_bool(const x_ini *d, const char *path_fmt, int dft_value, ...)
{
	va_list ap;
	va_start(ap, dft_value);
	char *value = x_ini_path_vget(d, path_fmt, ap);
	va_end(ap);
	if (!value)
		return dft_value;

	if (x_stricmp(value, "true") == 0)
		return 1;
	if (x_stricmp(value, "false") == 0)
		return 0;

	if (x_stricmp(value, "yes") == 0)
		return 1;
	if (x_stricmp(value, "no") == 0)
		return 0;

	if (x_stricmp(value, "y") == 0)
		return 1;
	if (x_stricmp(value, "n") == 0)
		return 0;

	if (x_stricmp(value, "1") == 0)
		return 1;
	if (x_stricmp(value, "0") == 0)
		return 0;

	return -1;
}

int x_ini_get_int(const x_ini *d, const char *path_fmt, int dft_value, ...)
{
	va_list ap;
	va_start(ap, dft_value);
	char *value = x_ini_path_vget(d, path_fmt, ap);
	va_end(ap);
	if (!value)
		return dft_value;
	int num;
	if (sscanf(value, "%d", &num) != 1)
		return dft_value;
	return num;
}

double x_ini_get_float(const x_ini *d, const char *path_fmt, double dft_value, ...)
{
	va_list ap;
	va_start(ap, dft_value);
	char *value = x_ini_path_vget(d, path_fmt, ap);
	va_end(ap);
	if (!value)
		return dft_value;
	double num;
	if (sscanf(value, "%lf", &num) != 1)
		return dft_value;
	return num;
}

char *x_ini_get_str(const x_ini *d, const char *path_fmt, char *dft_value, ...)
{
	va_list ap;
	va_start(ap, dft_value);
	char *value = x_ini_path_vget(d, path_fmt, ap);
	va_end(ap);
	if (!value)
		return dft_value;
	return value;
}

void x_ini_pure(x_ini *d)
{
	if (!d)
		return;
	x_list_foreach(cur_sec, &d->sec_list) {
		struct x_ini_section_st *sec = x_container_of(cur_sec, struct x_ini_section_st, link);
		free(sec->comment);
		sec->comment = NULL;

		for (x_link *cur_opt = x_list_first(&sec->opt_list); cur_opt != &sec->opt_list.head; ) {
			struct x_ini_option_st *opt = x_container_of(cur_opt, struct x_ini_option_st, link);
			cur_opt = cur_opt->next;
			free(opt->comment);
			opt->comment = NULL;
			if (opt->key) {
				continue;
			}
			x_list_del(&opt->link);
			free_option(opt);
		}
	}
}
