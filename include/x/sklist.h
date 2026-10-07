/*
 * Copyright (c) 2026 Li Xilin <lixilin@gmx.com>
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
#ifndef X_SKLIST_H
#define X_SKLIST_H

#include "linkage.h"
#include "list.h"
#include "memory.h"
#include "flowctl.h"

X_EXTERN_C_BEGIN

#define X_SKLIST_LEVEL_MAX 32

#define x_sklist_foreach(elem, sklist) \
	x_block_if ((sklist)->level) \
		x_list_foreach (__sklist_##elem, &(sklist)->list[0]) \
			x_block_var (void *elem = x_container_of(__sklist_##elem, x_sklink, link[0])->data)

typedef struct x_sklist_st x_sklist;
typedef struct x_sklink_st x_sklink;
typedef struct x_skpath_st x_skpath;
typedef int x_sklist_compare_fn(const void *p1, const void *p2);

struct x_sklist_st {
	x_mset mset;
	uint8_t level;
	uint32_t seed;
	x_sklist_compare_fn *compare;
	x_list list[X_SKLIST_LEVEL_MAX];
};

struct x_sklink_st {
	void *data;
	x_link link[];
};

struct x_skpath_st {
	x_link *link[X_SKLIST_LEVEL_MAX];
};

bool x_sklist_path_find(x_sklist *sklist, const void *data, x_skpath *path);
void x_sklist_path_insert(x_sklist *sklist, x_skpath *path, void *data);
void x_sklist_path_remove(x_sklist *sklist, x_skpath *path);
void x_sklist_init(x_sklist *sklist, x_sklist_compare_fn *compare);
int x_sklist_random_level(x_sklist *sklist);
void *x_sklist_find(x_sklist *sklist, const void *data);
void *x_sklist_find_or_insert(x_sklist *sklist, void *data);
void *x_sklist_replace_or_insert(x_sklist *sklist, void *data);
void *x_sklist_remove(x_sklist *sklist, const void *data);
x_dump *x_sklist_dump(x_sklist *sklist, x_dump *(*dump_value)(const void *data));
bool x_sklist_sanitize(x_sklist *sklist);
void x_sklist_free(x_sklist *sklist);

X_EXTERN_C_END

#endif

