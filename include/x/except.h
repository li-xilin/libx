/*
 * Copyright (c) 2026 Li Xilin <lixilin@gmx.com>
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
#ifndef X_EXCEPT_H
#define X_EXCEPT_H

#include <x/trick.h>
#include <x/flowctl.h>
#include <stdlib.h>
#include <stdio.h>

#define __X_EXCEPT_LABEL1(n) x_catch_##n
#define __X_EXCEPT_LABEL(n) __X_EXCEPT_LABEL1(n)

#define x_finish \
	jmp_buf __x_except_jmp_buf; \
	if (setjmp(__x_except_jmp_buf) == 0) \
		; \
	else \
		for (int __x_except_flag = 0; __x_except_flag < 2; __x_except_flag++) \
			if (__x_except_flag == 1) { \
				abort(); \
				goto __X_EXCEPT_LABEL(0); \
			} \
			else \
				__X_EXCEPT_LABEL(0):


#define x_catch(n) \
	if (setjmp(__x_except_jmp_buf) == 0) \
		; \
	else \
		__X_EXCEPT_LABEL(n): \
		for (int __x_except_flag = 0; __x_except_flag < 2; __x_except_flag++) \
			if (__x_except_flag == 1) { \
				goto __X_EXCEPT_LABEL(X_DEC(n)); \
				goto __X_EXCEPT_LABEL(n); \
			} \
			else 

#define x_throw (void)longjmp(__x_except_jmp_buf, 1);

#endif

