/*
 * Copyright (c) 2025-2026 Li Xilin <lixilin@gmx.com>
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
#ifndef X_REGEX_H
#define X_REGEX_H

#include "linkage.h"
#include "types.h"

X_EXTERN_C_BEGIN

#define X_REGEX_CATCH_MAX 16

struct x_recatch_st {
	struct {
		const char *str;
		int pos, len;
	} text;
	struct x_recatch_item_st {
		const char *ptr;
		int len;
		int id;
	} items [X_REGEX_CATCH_MAX];
	int idx, count;
};

int x_rematch(const char *txt, const char *re, x_recatch *c);
int x_recatch_size(x_recatch *c);
const char *x_recatch_at(x_recatch *c, int index);
int x_recatch_len_at(x_recatch *c, int index);
char *x_recatch_fetch(x_recatch *c, char *str, int index);
char *x_recatch_replace(x_recatch *c, char *new_str, const char *rpl_str, int id);
char *x_recatch_put(x_recatch *c, char *new_str, const char *put_str);

X_EXTERN_C_END

#endif


