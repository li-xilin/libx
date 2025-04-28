/*
 * Copyright (c) 2023-2024 Li Xilin <lixilin@gmx.com>
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

#include "x/tcolor.h"
#include "x/detect.h"
#include <stdio.h>
#include <assert.h>

#ifdef X_OS_WIN32
#include <windows.h>
#include <io.h>
static WORD s_fg_colors[] = {
	[X_TCOLOR_BLACK]   = 0,
	[X_TCOLOR_RED]     = FOREGROUND_RED,
	[X_TCOLOR_GREEN]   = FOREGROUND_GREEN,
	[X_TCOLOR_YELLOW]  = FOREGROUND_RED | FOREGROUND_GREEN,
	[X_TCOLOR_BLUE]    = FOREGROUND_BLUE,
	[X_TCOLOR_MAGENTA] = FOREGROUND_RED | FOREGROUND_BLUE,
	[X_TCOLOR_CYAN]    = FOREGROUND_GREEN | FOREGROUND_BLUE,
	[X_TCOLOR_WHITE]   = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE,

	[X_TCOLOR_GREY]     = FOREGROUND_INTENSITY,
	[X_TCOLOR_BRED]     = FOREGROUND_RED | FOREGROUND_INTENSITY,
	[X_TCOLOR_BGREEN]   = FOREGROUND_GREEN | FOREGROUND_INTENSITY,
	[X_TCOLOR_BYELLOW]  = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY,
	[X_TCOLOR_BBLUE]    = FOREGROUND_BLUE | FOREGROUND_INTENSITY,
	[X_TCOLOR_BMAGENTA] = FOREGROUND_RED | FOREGROUND_BLUE | FOREGROUND_INTENSITY,
	[X_TCOLOR_BCYAN]    = FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY,
	[X_TCOLOR_BWHITE]   = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY,
};

static WORD s_bg_colors[] = {
	[X_TCOLOR_BLACK]   = 0,
	[X_TCOLOR_RED]     = BACKGROUND_RED,
	[X_TCOLOR_GREEN]   = BACKGROUND_GREEN,
	[X_TCOLOR_YELLOW]  = BACKGROUND_RED | BACKGROUND_GREEN,
	[X_TCOLOR_BLUE]    = BACKGROUND_BLUE,
	[X_TCOLOR_MAGENTA] = BACKGROUND_RED | BACKGROUND_BLUE,
	[X_TCOLOR_CYAN]    = BACKGROUND_GREEN | BACKGROUND_BLUE,
	[X_TCOLOR_WHITE]   = BACKGROUND_RED | BACKGROUND_GREEN | BACKGROUND_BLUE,

	[X_TCOLOR_GREY]     = BACKGROUND_INTENSITY,
	[X_TCOLOR_BRED]     = BACKGROUND_RED | BACKGROUND_INTENSITY,
	[X_TCOLOR_BGREEN]   = BACKGROUND_GREEN | BACKGROUND_INTENSITY,
	[X_TCOLOR_BYELLOW]  = BACKGROUND_RED | BACKGROUND_GREEN | BACKGROUND_INTENSITY,
	[X_TCOLOR_BBLUE]    = BACKGROUND_BLUE | BACKGROUND_INTENSITY,
	[X_TCOLOR_BMAGENTA] = BACKGROUND_RED | BACKGROUND_BLUE | BACKGROUND_INTENSITY,
	[X_TCOLOR_BCYAN]    = BACKGROUND_GREEN | BACKGROUND_BLUE | BACKGROUND_INTENSITY,
	[X_TCOLOR_BWHITE]   = BACKGROUND_RED | BACKGROUND_GREEN | BACKGROUND_BLUE | BACKGROUND_INTENSITY,
};

#define COLOR_BITS (FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE)

#else
#include <unistd.h>

static const char *s_fg_colors[] = {
	[X_TCOLOR_BLACK]   = "\x1b[30m",
	[X_TCOLOR_RED]     = "\x1b[31m",
	[X_TCOLOR_GREEN]   = "\x1b[32m",
	[X_TCOLOR_YELLOW]  = "\x1b[33m",
	[X_TCOLOR_BLUE]    = "\x1b[34m",
	[X_TCOLOR_MAGENTA] = "\x1b[35m",
	[X_TCOLOR_CYAN]    = "\x1b[36m",
	[X_TCOLOR_WHITE]   = "\x1b[37m",

	[X_TCOLOR_GREY]     = "\x1b[90m",
	[X_TCOLOR_BRED]     = "\x1b[91m",
	[X_TCOLOR_BGREEN]   = "\x1b[92m",
	[X_TCOLOR_BYELLOW]  = "\x1b[93m",
	[X_TCOLOR_BBLUE]    = "\x1b[94m",
	[X_TCOLOR_BMAGENTA] = "\x1b[95m",
	[X_TCOLOR_BCYAN]    = "\x1b[96m",
	[X_TCOLOR_BWHITE]   = "\x1b[97m",
};

static const char *s_bg_colors[] = {
	[X_TCOLOR_BLACK]   = "\x1b[40m",
	[X_TCOLOR_RED]     = "\x1b[41m",
	[X_TCOLOR_GREEN]   = "\x1b[42m",
	[X_TCOLOR_YELLOW]  = "\x1b[43m",
	[X_TCOLOR_BLUE]    = "\x1b[44m",
	[X_TCOLOR_MAGENTA] = "\x1b[45m",
	[X_TCOLOR_CYAN]    = "\x1b[46m",
	[X_TCOLOR_WHITE]   = "\x1b[47m",

	[X_TCOLOR_GREY]     = "\x1b[100m",
	[X_TCOLOR_BRED]     = "\x1b[101m",
	[X_TCOLOR_BGREEN]   = "\x1b[102m",
	[X_TCOLOR_BYELLOW]  = "\x1b[103m",
	[X_TCOLOR_BBLUE]    = "\x1b[104m",
	[X_TCOLOR_BMAGENTA] = "\x1b[105m",
	[X_TCOLOR_BCYAN]    = "\x1b[106m",
	[X_TCOLOR_BWHITE]   = "\x1b[107m",
};
#endif

static int file_isatty(FILE *file)
{
#ifdef X_OS_WIN32
	return _isatty(_fileno(file));
#else
	return isatty(fileno(file));
#endif
}

void x_tcolor_set(FILE *file, x_tcolor *tc)
{
	assert(tc->fp == NULL);
	if (!file_isatty(file)) {
		tc->fp = NULL;
		return;
	}
#ifdef X_OS_WIN32
	CONSOLE_SCREEN_BUFFER_INFO csbi;
	HANDLE handle = (HANDLE)_get_osfhandle(_fileno(file));
	if (handle == INVALID_HANDLE_VALUE)
		return;

	handle = GetStdHandle(STD_ERROR_HANDLE);
	if (GetFileType(handle) != FILE_TYPE_CHAR)
		return;
	GetConsoleScreenBufferInfo(handle, &csbi);

	tc->attr = csbi.wAttributes;
	tc->bg_color = -1;
	tc->fg_color = -1;
#endif
	tc->fp = file;
}
void x_tcolor_reset(x_tcolor *tc)
{
	if (!tc->fp)
		return;
#ifdef X_OS_WIN32
	HANDLE handle = (HANDLE)_get_osfhandle(_fileno(tc->fp));
	if (handle == INVALID_HANDLE_VALUE)
		return;
	SetConsoleTextAttribute(handle, tc->attr);
#else
	fputs("\x1b[0m", tc->fp);
#endif
	tc->fp = NULL;
}
void x_tcolor_bold( x_tcolor *tc) /* Does not do anything on Windows */
{
	if (!tc->fp)
		return;
#ifdef X_OS_WIN32
	(void)tc;
#else
	fputs("\x1b[1m", tc->fp);
#endif
}
void x_tcolor_fg(x_tcolor *tc, int color)
{
	if (!tc->fp)
		return;
	if (color < 0)
		return;
#ifdef X_OS_WIN32
	HANDLE handle =(HANDLE) _get_osfhandle(_fileno(tc->fp));
	if (handle == INVALID_HANDLE_VALUE)
		return;
	tc->fg_color = color;
	WORD wAttr = (tc->attr & ~COLOR_BITS)
		| s_fg_colors[color]
		| (tc->bg_color < 0 ? 0 : s_bg_colors[tc->bg_color]);
	SetConsoleTextAttribute(handle, wAttr);
#else
	fputs(s_fg_colors[color], tc->fp);
#endif
}

void x_tcolor_bg(x_tcolor *tc, int color)
{
	if (!tc->fp)
		return;
	if (color < 0)
		return;
#ifdef X_OS_WIN32
	HANDLE handle =(HANDLE) _get_osfhandle(_fileno(tc->fp));
	if (handle == INVALID_HANDLE_VALUE)
		return;
	tc->fg_color = color;
	WORD wAttr = (tc->attr & ~COLOR_BITS)
		| s_bg_colors[color]
		| (tc->fg_color < 0 ? 0 : s_fg_colors[tc->bg_color]);
	SetConsoleTextAttribute(handle, wAttr);
#else
	fputs(s_bg_colors[color], tc->fp);
#endif
}
