/*
 * Copyright (c) 2024 Li Xilin <lixilin@gmx.com>
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

#ifndef X_SPLAY_H
#define X_SPLAY_H

#include <stddef.h>
#include <stdbool.h>

#ifndef X_BTNODE
#define X_BTNODE
typedef struct x_btnode_st x_btnode;
#endif

#ifndef X_SPLAY
#define X_SPLAY
typedef struct x_splay_st x_splay;
#endif

struct x_btnode_st
{
    x_btnode *parent, *left, *right;
};

typedef int x_splay_comp_f(const x_btnode *left, const x_btnode *right);

struct x_splay_st {
    x_btnode *root;
    x_splay_comp_f *comp;
    size_t size;
};

inline static void x_splay_init(x_splay *t, x_splay_comp_f *f)
{
	t->root = NULL;
	t->comp = f;
	t->size = 0;
}

inline static bool x_splay_linked(x_splay *t, const x_btnode *node)
{
	return node->left || node->right || node->parent || t->root == node;
}

x_btnode *x_splay_find(x_splay *t, const x_btnode *node);

x_btnode *x_splay_find_or_insert(x_splay *t, x_btnode *new);

void x_splay_replace(x_splay *t, x_btnode *old, x_btnode *new);

x_btnode *x_splay_replace_or_insert(x_splay *t, x_btnode *new);

void x_splay_remove(x_splay *t, x_btnode *node);

x_btnode *x_splay_first(x_splay *t);

x_btnode *x_splay_next(x_btnode *node);

x_btnode *x_splay_prev(x_btnode *node);

x_btnode *x_splay_last(x_splay *t);

#endif
