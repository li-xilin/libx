/*
 * Copyright (c) 2023 Li Xilin <lixilin@gmx.com>
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

#ifndef X_TCOLOR_H
#define X_TCOLOR_H

#include "detect.h"
#include "types.h"
#include <stdio.h>

enum {
	X_TCOLOR_BLACK = 0,
	X_TCOLOR_RED,
	X_TCOLOR_GREEN,
	X_TCOLOR_YELLOW,
	X_TCOLOR_BLUE,
	X_TCOLOR_MAGENTA,
	X_TCOLOR_CYAN,
	X_TCOLOR_WHITE,

	X_TCOLOR_GREY,
	X_TCOLOR_BRED,
	X_TCOLOR_BGREEN,
	X_TCOLOR_BYELLOW,
	X_TCOLOR_BBLUE,
	X_TCOLOR_BMAGENTA,
	X_TCOLOR_BCYAN,
	X_TCOLOR_BWHITE,
};

struct x_tcolor_st
{
	FILE *fp;
#ifdef X_OS_WIN
	uint16_t attr;
	int8_t bg_color;
	int8_t fg_color;
#endif
};

void x_tcolor_set(FILE *file, x_tcolor *tc);
void x_tcolor_reset(x_tcolor *tc);
void x_tcolor_bold( x_tcolor *tc); /* Does not do anything on Windows */
void x_tcolor_fg(x_tcolor *tc, int color);
void x_tcolor_bg(x_tcolor *tc, int color);

#endif

