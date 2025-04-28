/*
 * Copyright (c) 2020-2024 Li Xilin <lixilin@gmx.com>
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

#include "x/splay.h"
#include <stdlib.h>
#include <assert.h>

static void splay(x_splay *t, x_btnode *x);
static void check_sanity(x_splay *t);
static void rotate(x_btnode *child);
static x_btnode *leftmost(x_btnode *node);
static x_btnode *rightmost(x_btnode *node);
static void mark_gp(x_btnode *child);
inline static void zig(x_btnode *x, x_btnode *p);
inline static void zigzig(x_btnode *x, x_btnode *p);
inline static void zigzag(x_btnode *x, x_btnode *p);

static void splay(x_splay *t, x_btnode *x)
{
	while (1) {
		x_btnode *p = x->parent;
		if (!p) {
			t->root = x;
			return;
		}

		x_btnode *g = p->parent;
		if (!p->parent) {
			zig(x, p);
			continue;
		}
		if (x == p->left && p == g->left) {
			zigzig(x, p);
			continue;
		}
		if (x == p->right && p == g->right) {
			zigzig(x, p);
			continue;
		}
		zigzag(x, p);
	}
}

inline static void zig(x_btnode *x, x_btnode *p)
{
	rotate(x);
}

inline static void zigzig(x_btnode *x, x_btnode *p)
{
	rotate(p);
	rotate(x);
}

inline static void zigzag(x_btnode *x, x_btnode *p)
{
	rotate(x);
	rotate(x);
}

x_btnode *x_splay_find_or_insert(x_splay *t, x_btnode *new)
{
	x_btnode *removed = NULL;
	new->left = NULL;
	new->right = NULL;
	if (!t->root) {
		t->root = new;
		new->parent = NULL;
		goto out;
	}

	x_btnode *curr = t->root;
	x_btnode *parent;
	int left;
	while (curr) {
		parent = curr;
		int c = t->comp(new, curr);
		if (c < 0) {
			left = 1;
			curr = curr->left;
		}
		else if (c > 0) {
			left = 0;
			curr = curr->right;
		}
		else {
			return curr;
			// new = removed = curr;
			// goto out;
		}
	}
	new->parent = parent;
	if (left)
		parent->left = new;
	else
		parent->right = new;
out:
	splay(t, new);
	t->size++;
	return removed;
}

void x_splay_replace(x_splay *t, x_btnode *old, x_btnode *new)
{
	assert(t->comp(old, new) == 0);

	if (!old->parent) {
		assert(t->root == old);
		t->root = new;
		new->parent = NULL;
	} else {
		if (old->parent->left == old)
			old->parent->left = new;
		else
			old->parent->right = new;
		new->parent = old->parent;
	}
	if (old->left) {
		old->left->parent = new;
		new->left = old->left;
	}
	else
		new->left = NULL;

	if (old->left) {
		old->left->parent = new;
		new->left = old->left;
	}
	else
		new->left = NULL;

	if (old->right) {
		old->right->parent = new;
		new->right = old->right;
	}
	else
		new->right = NULL;
}

x_btnode *x_splay_replace_or_insert(x_splay *t, x_btnode *new)
{
	x_btnode *pNode = x_splay_find_or_insert(t, new);
	if (pNode) {
		x_splay_replace(t, pNode, new);
		return pNode;
	}
	return NULL;
}

x_btnode *x_splay_find(x_splay *t, const x_btnode *node)
{
	x_btnode *curr = t->root;
	while (curr != NULL) {
		int c = t->comp(node, curr);
		if (c < 0)
			curr = curr->left;
		else if (c > 0)
			curr = curr->right;
		else
			goto found;
	}
	return NULL;
found:
	splay(t, curr);
	return curr;
}

void x_splay_remove(x_splay *t, x_btnode *node)
{
	if (!node)
		return;
	splay(t, node);
	if (!node->left) {
		t->root = node->right;
		if (t->root != NULL)
			t->root->parent = NULL;
	}
	else if (!node->right) {
		t->root = node->left;
		t->root->parent = NULL;
	}
	else {
		x_btnode *x = leftmost(node->right);
		if (x->parent != node) {
			x->parent->left = x->right;
			if (x->right != NULL)
				x->right->parent = x->parent;
			x->right = node->right;
			x->right->parent = x;
		}
		t->root = x;
		x->parent = NULL;
		x->left = node->left;
		x->left->parent = x;
	}
	node->left = node->right = node->parent = NULL;
	t->size--;
	check_sanity(t);
}

x_btnode *x_splay_first(x_splay *t)
{
	return leftmost(t->root);
}

x_btnode *x_splay_next(x_btnode *node)
{
	if (node->right)
		return leftmost(node->right);
	while (node->parent && node == node->parent->right)
		node = node->parent;
	return node->parent;
}

x_btnode *x_splay_prev(x_btnode *node)
{
	if (node->left)
		return rightmost(node->left);
	while (node->parent && node == node->parent->left)
		node = node->parent;
	return node->parent;
}

x_btnode *x_splay_last(x_splay *t)
{
	return rightmost(t->root);
}

static void rotate(x_btnode *child)
{
	x_btnode *parent = child->parent;
	assert(parent != NULL);
	if (parent->left == child) {
		mark_gp(child);
		parent->left = child->right;
		if (child->right)
			child->right->parent = parent;
		child->right = parent;
	}
	else {
		mark_gp(child);
		parent->right = child->left;
		if (child->left)
			child->left->parent = parent;
		child->left = parent;
	}
}

static void mark_gp(x_btnode *child)
{
	x_btnode *parent = child->parent;
	x_btnode *grand = parent->parent;
	child->parent = grand;
	parent->parent = child;
	if (!grand)
		return;
	if (grand->left == parent)
		grand->left = child;
	else
		grand->right = child;
}

static x_btnode *leftmost(x_btnode *node)
{
	x_btnode *parent = NULL;
	while (node) {
		parent = node;
		node = node->left;
	}
	return parent;
}

static x_btnode *rightmost(x_btnode *node)
{
	x_btnode *parent = NULL;
	while (node) {
		parent = node;
		node = node->right;
	}
	return parent;
}

#ifndef NDEBUG
static int check_node_sanity(x_btnode *x, void *floor, void *ceil, x_splay_comp_f *comp)
{
	int count = 1;
	if (x->left) {
		assert(x->left->parent == x);
		void *new_floor;
		if (!floor || comp(x, floor) < 0)
			new_floor = x;
		else
			new_floor = floor;
		count += check_node_sanity(x->left, new_floor, ceil, comp);
	}
	if (x->right) {
		assert(x->right->parent == x);
		void *new_ceil;
		if (!ceil || comp(x, ceil) > 0)
			new_ceil = x;
		else
			new_ceil = ceil;
		count += check_node_sanity(x->right, floor, new_ceil, comp);
	}
	return count;
}
#endif

static void check_sanity(x_splay *t)
{
#ifndef NDEBUG
	if (!t->root)
		assert(t->size == 0);
	else {
		int reachable = check_node_sanity(t->root, NULL, NULL, t->comp);
		assert(reachable == t->size);
	}
#endif
}
