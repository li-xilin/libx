/*
 * Copyright (c) 2025 Li Xilin <lixilin@gmx.com>
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

#ifndef X_BTNODE_H
#define X_BTNODE_H

#include "types.h"

typedef int x_btnode_comp_f(const x_btnode *left, const x_btnode *right);

struct x_btnode_st
{
    x_btnode *parent, *left, *right;
};

x_btnode *x_btnode_first(x_btnode *node);
x_btnode *x_btnode_next(x_btnode *node);
x_btnode *x_btnode_prev(x_btnode *node);
x_btnode *x_btnode_last(x_btnode *node);
void x_btnode_zig(x_btnode *x);
void x_btnode_zigzig(x_btnode *x, x_btnode *p);
void x_btnode_zigzag(x_btnode *x, x_btnode *p);
void x_btnode_splay(x_btnode *x);
void x_btnode_join(x_btnode *x, x_btnode *y, x_btnode *out);
void x_btnode_rotate_right(x_btnode *x);
void x_btnode_rotate_left(x_btnode *x);
void x_btnode_rotate(x_btnode *x);
void x_btnode_insert_before(x_btnode *x, x_btnode *y);
void x_btnode_insert_after(x_btnode *x, x_btnode *y);

#endif

