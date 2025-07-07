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

#include "x/uchar.h"
#include "x/detect.h"
#include "x/unicode.h"

#include <string.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <assert.h>

#ifdef X_OS_WIN
#include <windows.h>
static int AnsiToWideChar(LPCSTR pAnsiStr, LPWSTR pUtf16Str, DWORD dwSize)
{
	int cBuf = MultiByteToWideChar(CP_ACP, 0, pAnsiStr, -1, NULL, 0);
	LPWSTR buf = HeapAlloc(GetProcessHeap(), 0, cBuf * sizeof(WCHAR));
	if (!buf) {
		errno = ENOMEM;
		return -1;
	}
	MultiByteToWideChar(CP_ACP, 0, pAnsiStr, -1, buf, cBuf);
	if (cBuf > dwSize / 2) {
		memcpy(pUtf16Str, buf, dwSize);
		errno = ENOMEM;
		HeapFree(GetProcessHeap(), 0, buf);
		return -1;
	}
	memcpy(pUtf16Str, buf, cBuf * 2);
	HeapFree(GetProcessHeap(), 0, buf);
	return cBuf;
}

static int WideCharToAnsi(LPCWSTR pUtf16Str, LPSTR pAnsiStr, DWORD dwSize)
{
	int cBuf = WideCharToMultiByte(CP_ACP, 0, pUtf16Str, -1, NULL, 0, NULL, NULL);
	LPSTR buf = HeapAlloc(GetProcessHeap(), 0, cBuf);
	if (!buf) {
		errno = ENOMEM;
		return -1;
	}
	WideCharToMultiByte(CP_ACP, 0, pUtf16Str, -1, buf, cBuf, NULL, NULL);

	if (cBuf > dwSize) {
		memcpy(pAnsiStr, buf, dwSize);
		HeapFree(GetProcessHeap(), 0, buf);
		errno = ENOMEM;
		return -1;
	}
	memcpy(pAnsiStr, buf, cBuf);
	HeapFree(GetProcessHeap(), 0, buf);
	return cBuf;
}
#endif

int x_utf8_to_ustr(const char *from, x_uchar *us, size_t size)
{
	int retval = -1;
	int utf8_len = strlen(from);
#ifdef X_OS_WIN
	int utf16_len = x_utf8_to_utf16(from, utf8_len, us, size);
	if (utf16_len == size) {
		errno = ENOBUFS;
	}
	else {
		us[utf16_len] = x_u('\0');
		retval = utf16_len;
	}
#else
	if (utf8_len > size - 1) {
		memcpy(us, from, size);
		errno = ENOBUFS;
	}
	else {
		memcpy(us, from, utf8_len);
		us[utf8_len] = x_u('\0');
		retval = utf8_len;
	}
#endif
	return retval;
}

int x_ansi_to_ustr(const char *from, x_uchar *us, size_t size)
{
#ifdef X_OS_WIN
	return AnsiToWideChar(from, us, size);
#else
	return x_utf8_to_ustr(from, us, size);
#endif
}

int x_utf16_to_ustr(const uint16_t *from, x_uchar *us, size_t size)
{
	int retval = -1;
	int utf16_len = x_utf16_strlen(from);
#ifdef X_OS_WIN
	if (utf16_len > size / 2 - 2) {
		memcpy(us, from, size);
		errno = ENOBUFS;
	}
	else {
		memcpy(us, from, (utf16_len + 1) * 2);
		retval = utf16_len;
	}
#else
	int utf8_len = x_utf16_to_utf8(from, utf16_len, us, size);
	if (utf8_len == size) {
		errno = ENOBUFS;
	}
	else {
		us[utf16_len] = x_u('\0');
		retval = utf16_len;
	}
#endif
	return retval;
}

int x_ustr_to_utf8(const x_uchar *us, char *to, size_t size)
{
#ifdef X_OS_WIN
	int retval = -1;
	int utf16_len = x_ustrlen(us);
	if (x_utf16_to_utf8(us, utf16_len, to, size) != utf16_len)
		errno = ENOBUFS;
	else {
		to[utf16_len] = '\0';
		retval = utf16_len;
	}
	return retval;
#else
	return x_utf8_to_ustr(us, to, size);
#endif
}

int x_ustr_to_ansi(const x_uchar *us, char *to, size_t size)
{
#ifdef X_OS_WIN
	return WideCharToAnsi(us, to, size);
#else
	return x_utf8_to_ustr(us, to, size);
#endif
}

int x_ustr_to_utf16(const x_uchar *us, uint16_t *to, size_t size)
{
#ifdef X_OS_WIN
	return x_utf16_to_ustr(us, to, size);
#else
	int retval = -1;
	int utf8_len = strlen(us);
	if (x_utf8_to_utf16(us, utf8_len, to, size) != utf8_len) {
		errno = ENOBUFS;
	}
	else {
		to[utf8_len] = x_u('\0');
		retval = utf8_len;
	}
	return retval;
#endif
}

x_uchar *x_ustrsplit(x_uchar **s, x_uchar ch)
{
	assert(s != NULL);
	x_uchar *ret;
	if (*s) {
		for (x_uchar *p = *s; *p; p++) {
			if (*p != ch)
				continue;
			*p = x_u('\0');
			ret = *s;
			*s = p + 1;
			return ret ;
		}
		ret = *s;
	}
	else {
		ret = NULL;
	}
	*s = NULL;
	return ret;
}

x_uchar *x_ustrdup(const x_uchar *s)
{
	size_t size = (x_ustrlen(s) + 1) * sizeof(x_uchar);
	x_uchar *dup = malloc(size);
	if (!dup)
		return NULL;
	memcpy(dup, s, size);
	return dup;
}

size_t x_ustrnihash(const x_uchar *s, size_t len)
{
	size_t h = 5381;
	for (int i = 0; i < len && s[i]; i++) {
		h = (h ^ (h << 5)) ^ tolower(s[i]);
	}
	return h;
}

size_t x_ustrihash(const x_uchar *s)
{
	size_t h = 5381;
	int c;
	while ((c = *s++)) {
		h = (h ^ (h << 5)) ^ tolower(c);
	}
	return h;
}

int x_umain(x_umain_fn *umain, int argc, char *argv[])
{
#ifdef X_OS_WIN
	int nArgs;
	LPWSTR *pArgList = CommandLineToArgvW(GetCommandLineW(), &nArgs);
	if (!pArgList)
		return 1;
	return umain(nArgs, pArgList);
#else
	return umain(argc, argv);
#endif
}

