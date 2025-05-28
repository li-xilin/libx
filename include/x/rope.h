/*
 * Copyright (c) 2025 Li Xilin <lixilin@gmx.com>
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
#ifndef X_ROPE_H
#define X_ROPE_H

#include "types.h"
#include "btnode.h"
#include "macros.h"

#define X_ROPE_SPLIT_SIZE 4096

struct x_rope_node_st {
	x_btnode node;
	size_t weight, size;
	char *ptr;
};

struct x_rope_st {
	x_rope_node *root_node;
};

int x_rope_init(x_rope *r, const char *str);
int x_rope_init1(x_rope *r, char *ptr, size_t len);
void x_rope_free(x_rope *r);
const x_rope_node *x_rope_get_node(x_rope *r, size_t index);
void x_rope_merge(x_rope *r, x_rope *r1);
void x_rope_swap(x_rope *r, x_rope *r1);
int x_rope_append(x_rope *r, const char *str);
int x_rope_balance(x_rope *r);
int x_rope_clone(const x_rope *r, x_rope *out);
int x_rope_split(x_rope *ri, size_t index, x_rope *ro);
int x_rope_insert(x_rope *r, size_t index, x_rope *ins);
int x_rope_remove(x_rope *rio, size_t index, size_t length, x_rope *out);
size_t x_rope_length(const x_rope *ri);
char *x_rope_at(const x_rope *ri, size_t index);
char *x_rope_splice(const x_rope *r);
void x_rope_dump_seq(const x_rope *r, void *fp);
void x_rope_dump_tree(const x_rope *r, void *fp);
int x_rope_vprintf(x_rope *r, size_t index, const char *fmt, va_list ap);
int x_rope_printf(x_rope *r, size_t index, const char *fmt, ...);
int x_rope_balance(x_rope *r);

#define x_rope_foreach(var, rope) \
	for (x_rope_node *var = x_container_of(x_btnode_first(&(rope)->root_node->node), x_rope_node, node); \
			(&(var)->node) != NULL; \
			(var) = x_container_of(x_btnode_next(&(var)->node), x_rope_node, node))


#endif
