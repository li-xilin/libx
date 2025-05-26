/*
 * Copyright (c) 2021-2024 Li Xilin <lixilin@gmx.com>
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

#ifndef X_STRING_H
#define X_STRING_H

#include "def.h"
#include <string.h>

#define x_swap(a, b, type) \
do { \
	type *x = a, *y = b; \
	type tmp = *x; \
	*(x) = *(y); \
	*(y) = tmp; \
} while(0)

inline static uint32_t x_hash32(const void *key, size_t size)
{
	uint32_t h = 3323198485ul;
	for (size_t i = 0; i < size; i++) {
		h ^= ((uint8_t *)key)[i];
		h *= 0x5bd1e995;
		h ^= h >> 15;
	}
	return h;
}

inline static uint64_t x_hash64(const void *key, size_t size)
{
	uint64_t h = 525201411107845655ull;
	for (size_t i = 0; i < size; i++) {
		h ^= ((uint8_t *)key)[i];
		h *= 0x5bd1e9955bd1e995;
		h ^= h >> 47;
	}
	return h;
}

void x_memswp(void *p1, void *p2, size_t size);

char *x_strdup(const char *str);

char *x_strdup2(const char *s, size_t *lenp);

wchar_t *x_wcsdup(const wchar_t *str);

void *x_memdup(const void *p, size_t size);

size_t x_strhash(const char *s);

size_t x_wcshash(const wchar_t *s);

size_t x_memhash(const void *p, size_t size);

size_t x_strnihash(const char *s, size_t len);

size_t x_strihash(const char *s);

char *x_strsplit(char **s, char ch);

size_t x_strtoargv(char *s, char *argv[], size_t len);

char *x_strrepl(const char *orig, const char *rep, const char *with);

wchar_t *x_wcsrepl(const wchar_t *orig, const wchar_t *rep, const wchar_t *with);

char *x_memtoustr(const void *p, size_t size, char *buf);

char *x_memtohex(const void *p, size_t size, char *out);

void x_membyhex(const char *text, void *out);

char **x_strargv(const char *cmdline, int *count);

wchar_t **x_wcsargv(const wchar_t *cmdline, int *count);

void x_memxor(void *a, const void *b, size_t size);

char *x_strbaseconv(char *s, char *buf, size_t size, int old_base, int new_base);

char *x_strtrim(char *s);

wchar_t *x_wcstrim(wchar_t *s);

int x_stricmp(const char *s1, const char *s2);

int x_strnicmp(const char *s1, const char *s2, size_t len);

char *x_strnstr(const char *s, const char *find, size_t slen);

int x_strnrcmp(const char *s1, const char *s2, size_t len);

#endif

