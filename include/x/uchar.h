/*
 * Copyright (c) 2023,2025 Li Xilin <lixilin@gmx.com>
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

#ifndef X_UCHAR_H
#define X_UCHAR_H

#include "detect.h"
#include "unicode.h"
#include "assert.h"
#include <string.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>

#ifdef X_OS_WIN
#  include <windef.h>
#  include <winbase.h>
#  include <shellapi.h>
#  define __x_u(s) L##s
#  define x_ustrlen wcslen
#  define x_ustrcat wcscat
#  define x_ustrcpy wcscpy
#  define x_ustrncpy wcsncpy
#  define x_ustrstr wcsstr
#  define x_ustrchr wcschr
#  define x_ustrrchr wcsrchr
#  define x_ustrcmp wcscmp
#  define x_ustricmp _wcsicmp
#  define x_ustrnicmp _wcsnicmp
#  define x_ustrncmp wcsncmp

#  define x_usscanf swscanf

#  define x_ustrhash x_wcshash
#  define x_ustrargv x_wcsargv
#  define x_ustrtrim x_wcstrim
#  define x_ustr_index x_utf16_index
#  define x_ustr_to_ucode x_utf16_to_ucode
#  define x_ustr_charcnt x_utf16_charcnt
#  define x_ustr_charlen x_utf16_charlen
#  define x_ucode_to_ustr x_ucode_to_utf16
#  define x_ustrtoint _wtoi
#  define X_PRIus "ls"
#  define X_PRIs "hs"
#  define X_PRIuc "lc"
#  define X_PRIc "hc"
#  define X_UCHAR_BYTES 2
typedef WCHAR x_uchar;
#else
#  define __x_u(s) s
#  define x_ustrlen strlen
#  define x_ustrcat strcat
#  define x_ustrcpy strcpy
#  define x_ustrncpy strncpy
#  define x_ustrstr strstr
#  define x_ustrchr strchr
#  define x_ustrrchr strrchr
#  define x_ustrcmp strcmp
#  define x_ustrncmp strncmp
#  define x_ustricmp strcasecmp
#  define x_ustrnicmp strncasecmp

#  define x_usscanf sscanf

#  define x_ustrhash x_strhash
#  define x_ustrargv x_strargv
#  define x_ustrtrim x_strtrim
#  define x_ustr_index x_utf8_index
#  define x_ustr_to_ucode x_utf8_to_ucode
#  define x_ustr_charcnt x_utf8_charcnt
#  define x_ustr_charlen x_utf8_charlen
#  define x_ucode_to_ustr x_ucode_to_utf8
#  define x_ustrtoint atoi
#  define X_PRIus "s"
#  define X_PRIs "s"
#  define X_PRIuc "c"
#  define X_PRIc "c"
#  define X_UCHAR_BYTES 1
typedef char x_uchar;
#endif

#define x_u(s) __x_u(s)

#define x_main \
	main(int argc, char *argv[]) { \
		extern int x_main(int argc, x_uchar *argv[]); \
		return x_umain(x_main, argc, argv);  \
	} \
	extern int x_main

#define x_uconv(s) __x_uconv(sizeof s[0], s, s[0])

typedef int x_umain_fn(int argc, x_uchar *argv[]);

int x_utf8_to_ustr(const char *from, x_uchar *us, size_t size);
int x_ansi_to_ustr(const char *from, x_uchar *us, size_t size);
int x_utf16_to_ustr(const uint16_t *from, x_uchar *us, size_t size);
int x_ustr_to_utf8(const x_uchar *us, char *to, size_t size);
int x_ustr_to_ansi(const x_uchar *us, char *to, size_t size);
int x_ustr_to_utf16(const x_uchar *us, uint16_t *to, size_t size);
x_uchar *x_ustrsplit(x_uchar **s, x_uchar ch);
x_uchar *x_ustrdup(const x_uchar *s);
size_t x_ustrihash(const x_uchar *s);
size_t x_ustrnihash(const x_uchar *s, size_t len);
x_uchar *x_utss_utf8(char *utf8);
x_uchar *x_utss_utf16(wchar_t *utf16);
int x_umain(x_umain_fn *umain, int argc, char *argv[]);

struct __x_check_bytesize { int i1: 1, i2: 2, i4: 4, i8: 8; };
static inline x_uchar *__x_uconv(int char_size, void *s, wchar_t test)
{
	x_static_assert(char_size == 1 || char_size == 2);
	switch (char_size) {
		case 1: return x_utss_utf8((char *)s);
		case 2: return x_utss_utf16((wchar_t *)s);
		default: return NULL;
	}
}

#endif

