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

#include "x/string.h"
#include "x/assert.h"
#include <stdlib.h>
#include <string.h>
#include <wchar.h>
#include <errno.h>
#include <ctype.h>
#include <stdio.h>
#include <assert.h>

void x_memswp(void *p1, void *p2, size_t size)
{
	assert(p1);
	assert(p2);

	
	size_t chunk_cnt = size / sizeof(uint_fast32_t),
	       tail_size = size % sizeof(uint_fast32_t);

	uint_fast32_t *pf1 = p1, *pf2 = p2;
	for (size_t i = 0; i < chunk_cnt; i++)
		x_swap(pf1 + i, pf2 + i, uint_fast32_t);

	uint8_t *ps1 = (uint8_t *)p1 + (size - tail_size),
		*ps2 = (uint8_t *)p2 + (size - tail_size);
	for (size_t i = 0; i != tail_size; i++)
		x_swap(ps1 + i, ps2 + i, uint8_t);
}

char *x_strdup(const char *s)
{
	assert(s);

	size_t size = (strlen(s) + 1) * sizeof(char);
	char *copy = malloc(size);
	if (!copy)
		return NULL;
	memcpy(copy, s, size);
	return copy;
}

char *x_strdup2(const char *s, size_t *lenp)
{
	assert(s);

	size_t len = strlen(s);
	size_t size = (len + 1) * sizeof(char);
	char *copy = malloc(size);
	if (!copy)
		return NULL;
	memcpy(copy, s, size);
	*lenp = len;
	return copy;
}


wchar_t *x_wcsdup(const wchar_t* s)
{
	assert(s);

	size_t size = (wcslen(s) + 1) * sizeof(wchar_t);
	wchar_t *copy = malloc(size);
	if (!copy)
		return NULL;
	memcpy(copy, s, size);
	return copy;
}

void *x_memdup(const void *p, size_t size)
{
	uint8_t *copy = malloc(size ? size : 1);
	if (!copy)
		return NULL;
	memcpy(copy, p, size);
	return copy;
}

size_t x_strhash(const char *s)
{
	assert(s);

	register size_t h = 5381;
	int c;
	while ((c = *s++)) {
		h = (h ^ (h << 5)) ^ c; /* hash * 33 + c */
	}
	if (h == 0)
		h = 1;
	return h;
}

size_t x_wcshash(const wchar_t *s)
{
	assert(s);

	register size_t h = 5381;
	int c;
	while ((c = *s++)) {
		h = (h ^ (h << 5)) ^ c; /* hash * 33 + c */
	}
	if (h == 0)
		h = 1;
	return h;
}

size_t x_memhash(const void *p, size_t size)
{
	assert(p);

	register size_t h = 5381;
	for (size_t i = 0; i < size; i++) {
		h = (h ^ (h << 5)) ^ ((uint8_t *)p)[i];
	}
	if (h == 0)
		h = 1;
	return h;
}

char *x_strsplit(char **s, char ch)
{
	assert(s);

	char *ret;
	if (*s) {
		for (char *p = *s; *p; p++) {
			if (*p != ch)
				continue;
			*p = '\0';
			ret = *s;
			*s = p + 1;
			return ret ;
		}
		ret = *s;
	} else {
		ret = NULL;
	}
	*s = NULL;
	return ret;
}

char *x_strrepl(const char *orig, const char *rep, const char *with)
{
	assert(orig);
	assert(rep);
	assert(with);
	x_assert(rep[0], "length of parameter rep is 0");

	const char *ins;  /* the next insert point */
	char *result;     /* the return string */
	char *tmp;        /* varies */

	size_t len_rep,   /* length of rep (the string to remove) */
	       len_with,  /* length of with (the string to replace rep with) */
	       len_front, /* distance between rep and end of last rep */
	       count;     /* number of replacements */

	len_rep = strlen(rep);
	if (len_rep == 0)
		return NULL; /* empty rep causes infinite loop during count */
	len_with = strlen(with);

	/* count the number of replacements needed */
	ins = orig;
	for (count = 0; (tmp = strstr(ins, rep)); ++count) {
		ins = tmp + len_rep;
	}

	tmp = result = malloc(strlen(orig) + (len_with - len_rep) * count + 1);

	if (!result)
		return NULL;

	/* first time through the loop, all the variable are set correctly
	 * from here on,
	 *    tmp points to the end of the result string
	 *    ins points to the next occurrence of rep in orig
	 *    orig points to the remainder of orig after "end of rep" */
	while (count--) {
		ins = strstr(orig, rep);
		len_front = ins - orig;
		tmp = strncpy(tmp, orig, len_front) + len_front;
		tmp = strcpy(tmp, with) + len_with;
		orig += len_front + len_rep; // move to next "end of rep"
	}
	strcpy(tmp, orig);
	return result;
}

wchar_t *x_wcsrepl(const wchar_t *orig, const wchar_t *rep, const wchar_t *with)
{
	assert(orig);
	assert(rep);
	assert(with);
	x_assert(rep[0], "length of parameter rep is 0");

	const wchar_t *ins;
	wchar_t *result, *tmp;
	size_t len_rep, len_with, len_front, count;

	len_rep = wcslen(rep);
	if (len_rep == 0)
		return NULL;
	len_with = wcslen(with);

	ins = orig;
	for (count = 0; (tmp = wcsstr(ins, rep)); ++count) {
		ins = tmp + len_rep;
	}

	tmp = result = malloc((wcslen(orig) + (len_with - len_rep) * count + 1) * sizeof(wchar_t));
	if (!result)
		return NULL;

	while (count--) {
		ins = wcsstr(orig, rep);
		len_front = ins - orig;
		tmp = wcsncpy(tmp, orig, len_front) + len_front;
		tmp = wcscpy(tmp, with) + len_with;
		orig += len_front + len_rep;
	}
	wcscpy(tmp, orig);
	return result;
}

inline static void char2hex(char dst[2], uint8_t src)
{
	uint8_t major = (src >> 4) , minor = src & 0x0F;
	dst[0] = (major < 0xA) ? '0' + major : 'A' + (major - 0xA);
	dst[1] = (minor < 0xA) ? '0' + minor : 'A' + (minor - 0xA);
}

char *x_memtohex(const void *p, size_t size, char *out)
{
	for (int i = 0; i < size; i++)
		char2hex(out + i * 2, ((uint8_t *)p)[i]);
	out[size * 2] = '\0';
	return out;
}

void x_membyhex(const char *text, void *out)
{
	char *buf = out;
	int i = 0;
	for (i = 0; text[i] != '\0'; i++)
		x_assert(isdigit((int)text[i]) || (text[i] >= 'A' && text[i] <= 'Z'), "invalid charactor '%c'", text[i]);
	x_assert(i % 2 == 0, "length of text is odd number");

	for (i = 0; text[i] != '\0'; i += 2)
		buf[i / 2] = (text[i] - (isdigit((int)text[i]) ? '0' : ('A' - 0xA))) * 0x10
			+ (text[i + 1] - (isdigit((int)text[i + 1]) ? '0' : ('A' - 0xA)));
}

size_t x_strtoargv(char *s, char *argv[], size_t argc_mx)
{
	assert(s);
	x_assert(argc_mx > 0, "argc_mx must be positive integer");
	char *next_ptr = s, *item;
	size_t count = 0;
	while (count < argc_mx - 1 && (item = x_strsplit(&next_ptr, ' '))) {
		if (*item == '\0')
			continue;
		argv[count] = item;
		count++;
	}
	argv[count] = NULL;
	return count;
}

void x_memxor(void *a, const void *b, size_t size)
{
	int i, j;
	for (i = 0; i < size / sizeof(long); i++)
		((long *)a)[i] ^= ((long *)b)[i];

	for (j = i * sizeof(long); j < size; j++)
		((char *)a)[j] ^= ((char *)b)[j];
}

inline static int char_to_int(char c)
{
        if (c >= '0' && c <= '9')
                return c - '0';
        if (c>= 'a' && c <= 'z')
                return c - 'a' + 0xA;
        if (c>= 'A' && c <= 'Z')
                return c - 'A' + 0xA;
        return -1;
}

inline static char int_to_char(int n)
{
        if (n >= 0 && n < 10)
                return '0' + n;
        if (n >= 10 && n < 36)
                return 'a' + n - 10;
        return '\0';
}

char *x_strbaseconv(char *s, char *buf, size_t size, int old_base, int new_base)
{
	int j = size - 2;
	x_assert(size >= 2, "invalid size");
	x_assert(old_base >= 2 && old_base <= 36, "invalid old_base");
	x_assert(new_base >= 2 && new_base <= 36, "invalid new_base");

	buf[size - 1] = '\0';
	for (int i = 0; s[i]; i++) {
                uint8_t n = char_to_int(s[i]);
                if (n < 0 || n >= old_base) {
			errno = EINVAL;
			return NULL;
		}
        }
        static const char hex[] = "0123456789abcdefghijklmnopqrstuvwxyz";
        while (1) {
                int i = 0;
                while (s[i] && s[i] == '0')
                        i++;
                if (!s[i])
                        break;
                if (j < 0) {
			errno = ENOBUFS;
                        return NULL;
		}
                int sum = 0, res = 0;
                for (i = 0; s[i]; i++) {
                        sum = res * old_base + char_to_int(s[i]);
                        s[i] = hex[sum / new_base];
                        res = sum % new_base;
                }
                buf[j--] = int_to_char(sum % new_base);
        }

        return buf + j + 1;
}

char *x_strtrim(char *s)
{
	int begin = 0, end = -1, i;
	for (i = 0; s[i] && isspace(s[i]); i++);
	begin = i;

	for (; s[i]; i++) {
		if (!isspace(s[i]))
			end = i;
	}

	if (end >= 0)
		s[end + 1] = '\0';
	return s + begin;
}

wchar_t *x_wcstrim(wchar_t *s)
{
	int begin = 0, end = -1, i;
	for (i = 0; s[i] && isspace(s[i]); i++);
	begin = i;

	for (; s[i]; i++) {
		if (!isspace(s[i]))
			end = i;
	}

	if (end >= 0)
		s[end + 1] = L'\0';
	return s + begin;
}

size_t x_strnihash(const char *s, size_t len)
{
	size_t h = 5381;
	for (int i = 0; i < len && s[i]; i++) {
		h = (h ^ (h << 5)) ^ tolower(s[i]);
	}
	return h;
}

size_t x_strihash(const char *s)
{
	size_t h = 5381;
	int c;
	while ((c = *s++)) {
		h = (h ^ (h << 5)) ^ tolower(c);
	}
	return h;
}

int x_stricmp(const char *s1, const char *s2)
{
	int i;
	for (i = 0; s1[i] && s2[i] && tolower(s1[i]) == tolower(s2[i]); i++);
	return tolower(s1[i]) - tolower(s2[i]);
}

int x_strnicmp(const char *s1, const char *s2, size_t len)
{
	int i;
	for (i = 0; i < len && s1[i] && s2[i] && tolower(s1[i]) == tolower(s2[i]); i++);
	if (i == len)
		return 0;
	return tolower(s1[i]) - tolower(s2[i]);
}

char *x_strnstr(const char *s, const char *find, size_t slen)
{
	char c, sc;
	size_t len;

	if ((c = *find++) != '\0') {
		len = strlen(find);
		do {
			do {
				if (slen-- < 1 || (sc = *s++) == '\0')
					return (NULL);
			} while (sc != c);
			if (len > slen)
				return (NULL);
		} while (strncmp(s, find, len) != 0);
		s--;
	}
	return ((char *)s);
}

int x_strnrcmp(const char *s1, const char *s2, size_t len)
{
	long len1 = strlen(s1);
	long len2 = strlen(s2);
	while (len1 && len2 && len) {
		int cmp = s1[len1 - 1] - s2[len2 - 1];
		if (cmp)
			return cmp;
		len1--, len2--, len--;
	}

	if (!len)
		return 0;

	int style = (!!len1 << 4) | !!len2;
	switch (style) {
		case 0x00:
			return 0;
		case 0x01:
			return -s2[len2 - 1];
		case 0x10:
			return s1[len1 - 1];
		case 0x11:
			return s1[len1 - 1] - s2[len2 - 1];
		default:
			abort();
	}
}

