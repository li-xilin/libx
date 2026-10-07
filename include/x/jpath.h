/*
 * Copyright (c) 2025-2026 Li Xilin <lixilin@gmx.com>
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
#ifndef X_JPATH__H
#define X_JPATH__H

#include "linkage.h"
#include "types.h"
#include "json.h"
#include "memory.h"
#include "flowctl.h"

X_EXTERN_C_BEGIN

enum x_jpath_type {
    X_JPATH_INVALID = 0,
    X_JPATH_START,
    X_JPATH_KEY,
    X_JPATH_INDEX,
    X_JPATH_ANYKEY,
    X_JPATH_ANYINDEX,
    X_JPATH_ANY,
};

struct x_jpath_st
{
    x_jpath *next;
    enum x_jpath_type type;
    char *key;
    int index;
};

struct x_jpath_result_node_st
{
    x_jpath_result_node *next;
    x_json *node;
};

struct x_jpath_result_st
{
    x_jpath_result_node *head;
    int len;
};

x_jpath *x_jpath_parse(const char *data, size_t *pos);
bool x_jpath_wildcarded(x_jpath *jp);
bool x_jpath_match(x_json* item, x_jpath* jp, bool case_sensitive, x_jpath_result* res);
bool x_jpath_insert(x_json* root, x_jpath* jp, bool case_sensitive, x_json* value);

x_jpath *x_jpath_create(enum x_jpath_type type);
void x_jpath_free(x_jpath *head);
bool x_jpath_append(x_jpath *jp, x_jpath *item);

x_jpath_result *x_jpath_result_create(void);
void x_jpath_result_free(x_jpath_result *res);
x_json *x_jpath_result_to_array(x_jpath_result *res);
bool x_jpath_result_add(x_jpath_result *res, x_json *item);

#define x_jpath_result_foreach(i, res) \
	for (x_jpath_result_node *__x_jpath_cur = (res)->head->next; __x_jpath_cur; __x_jpath_cur = __x_jpath_cur->next) \
		x_block_var(x_json *i = __x_jpath_cur->node)

X_EXTERN_C_END

#endif

