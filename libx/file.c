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

#include "x/detect.h"
#if defined(X_OS_WIN) && defined(X_CC_MINGW)
/* https://sourceforge.net/p/mingw-w64/bugs/846/ */
#  ifdef _INC_STDIO
#    error "x/file.h should be placed before stdandard stdio.h"
#  else
#    undef __USE_MINGW_ANSI_STDIO
#    define __USE_MINGW_ANSI_STDIO 0
#  endif
#endif
#include "x/file.h"
#include "x/errno.h"
#include "x/detect.h"
#include "x/uchar.h"
#include <stdio.h>
#include <fcntl.h>
#ifdef X_OS_WIN
#include <io.h>
#else
#include <unistd.h>
#endif

FILE* x_fdopen(intptr_t handle, const x_uchar *mode)
{
#ifdef X_OS_WIN
	int fd = _open_osfhandle(handle, _O_BINARY);
	if (fd != -1)
		return _wfdopen(fd, mode);
	else
		return NULL;
#else
	FILE *fp = fdopen(handle, mode);
	if (!fp) {
		x_eval_errno();
		return NULL;
	}
	return 0;
#endif
}

int x_setmode_utf8(FILE *stream)
{
	fflush(stream);
#ifdef X_OS_WIN
	fflush(stream);
	return _setmode(fileno(stream), _O_U8TEXT);
#else
	return 0;
#endif
}

int x_setmode_utf16(FILE *stream)
{
	fflush(stream);
#ifdef X_OS_WIN
	fflush(stream);
	return _setmode(fileno(stream), _O_U16TEXT);
#else
	return 0;
#endif
}

int x_setmode_binary(FILE *stream)
{
	fflush(stream);
#ifdef X_OS_WIN
	fflush(stream);
	return _setmode(fileno(stream), _O_BINARY);
#else
	return 0;
#endif
}

int x_isatty(FILE *file)
{
#ifdef X_OS_WIN32
	return _isatty(_fileno(file));
#else
	return isatty(fileno(file));
#endif
}

#include "uchar.h"
#include <stdint.h>
#include <stdio.h>
#include <stdarg.h>

FILE* x_fdopen(intptr_t handle, const x_uchar *mode);
int x_setinput(FILE *stream);
int x_setoutput(FILE *stream);
int x_isatty(FILE *file);

FILE *x_fopen(const x_uchar *path, const x_uchar *mode)
{
#ifdef X_OS_WIN
	return _wfopen(path, mode);
#else
	return fopen(path, mode);
#endif
}

FILE *x_freopen(const x_uchar *path, const x_uchar *mode, FILE *stream)
{
#ifdef X_OS_WIN
	return _wfreopen(path, mode, stream);
#else
	return freopen(path, mode, stream);
#endif
}

int x_vfprintf(FILE *stream, const x_uchar *format, va_list ap)
{
#ifdef X_OS_WIN
	return vfwprintf(stream, format, ap);
#else
	return vfprintf(stream, format, ap);
#endif
}

int x_fprintf(FILE *stream, const x_uchar *format, ...)
{
	va_list ap;
	va_start(ap, format);
	int ret = x_vfprintf(stream, format, ap);
	va_end(ap);
	return ret;
}

int x_vprintf(const x_uchar *format, va_list ap)
{
#ifdef X_OS_WIN
	return vwprintf(format, ap);
#else
	return vprintf(format, ap);
#endif
}

int x_printf(const x_uchar *format, ...)
{
	va_list ap;
	va_start(ap, format);
	int ret = x_vprintf(format, ap);
	va_end(ap);
	return ret;
}

int x_fputs(const x_uchar *ws, FILE *stream)
{
#ifdef X_OS_WIN
	return fputws(ws, stream);
#else
	return fputs(ws, stream);
#endif
}

int x_fputc(x_uchar c, FILE *stream)
{
#ifdef X_OS_WIN
	return fputwc(c, stream);
#else
	return fputc(c, stream);
#endif
}

int x_putchar(x_uchar c)
{
#ifdef X_OS_WIN
	return fputwc(c, stdout);
#else
	return fputc(c, stdout);
#endif
}

int x_vfscanf(FILE *stream, const x_uchar *format, va_list ap)
{
#ifdef X_OS_WIN
	return vfwscanf(stream, format, ap);
#else
	return vfscanf(stream, format, ap);
#endif
}

int x_fscanf(FILE *stream, const x_uchar *format, ...)
{
	va_list ap;
	va_start(ap, format);
	int ret = x_vfscanf(stream, format, ap);
	va_end(ap);
	return ret;
}

int x_vscanf(const x_uchar *format, va_list ap)
{
#ifdef X_OS_WIN
	return vwscanf(format, ap);
#else
	return vscanf(format, ap);
#endif
}

int x_scanf(const x_uchar *format, ...)
{
	va_list ap;
	va_start(ap, format);
	int ret = x_vscanf(format, ap);
	va_end(ap);
	return ret;
}

x_uchar *x_fgets(x_uchar *ws, int n, FILE *stream)
{
#ifdef X_OS_WIN
	return fgetws(ws, n, stream);
#else
	return fgets(ws, n, stream);
#endif
}

int x_fgetc(FILE *stream)
{
#ifdef X_OS_WIN
	return fgetwc(stream);
#else
	return fgetc(stream);
#endif
}

int x_getchar(void)
{
#ifdef X_OS_WIN
	return getwchar();
#else
	return getchar();
#endif
}
