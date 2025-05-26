#include "x/btnode.h"
#include <assert.h>

static void mark_gp(x_btnode *child);

x_btnode *x_btnode_first(x_btnode *node)
{
	x_btnode *parent = NULL;
	while (node) {
		parent = node;
		node = node->left;
	}
	return parent;
}

x_btnode *x_btnode_last(x_btnode *node)
{
	x_btnode *parent = NULL;
	while (node) {
		parent = node;
		node = node->right;
	}
	return parent;
}

x_btnode *x_btnode_next(x_btnode *node)
{
	if (node->right)
		return x_btnode_first(node->right);
	while (node->parent && node == node->parent->right)
		node = node->parent;
	return node->parent;
}

x_btnode *x_btnode_prev(x_btnode *node)
{
	if (node->left)
		return x_btnode_last(node->left);
	while (node->parent && node == node->parent->left)
		node = node->parent;
	return node->parent;
}

void x_btnode_join(x_btnode *x, x_btnode *y, x_btnode *out)
{
	x_btnode *lnode = x_btnode_last(x);
	x_btnode *rnode = x_btnode_first(x);
	x_btnode_splay(lnode);
	x_btnode_splay(rnode);
	out->parent = NULL;
	out->left = lnode;
	out->right = rnode;
}

void x_btnode_splay(x_btnode *x)
{
	while (1) {
		x_btnode *p = x->parent;
		if (!p)
			break;
		x_btnode *g = p->parent;
		if (!p->parent) {
			x_btnode_zig(x);
			continue;
		}
		if (x == p->left && p == g->left) {
			x_btnode_zigzig(x, p);
			continue;
		}
		if (x == p->right && p == g->right) {
			x_btnode_zigzig(x, p);
			continue;
		}
		x_btnode_zigzag(x, p);
	}
}

void x_btnode_zig(x_btnode *x)
{
	x_btnode_rotate(x);
}

void x_btnode_zigzig(x_btnode *x, x_btnode *p)
{
	x_btnode_rotate(p);
	x_btnode_rotate(x);
}

void x_btnode_zigzag(x_btnode *x, x_btnode *p)
{
	x_btnode_rotate(x);
	x_btnode_rotate(x);
}

void x_btnode_rotate_right(x_btnode *x)
{
	x_btnode *parent = x->parent;
	mark_gp(x);
	parent->left = x->right;
	if (x->right)
		x->right->parent = parent;
	x->right = parent;
}

void x_btnode_rotate_left(x_btnode *x)
{
	x_btnode *parent = x->parent;
	mark_gp(x);
	parent->right = x->left;
	if (x->left)
		x->left->parent = parent;
	x->left = parent;
}

void x_btnode_rotate(x_btnode *x)
{
	x_btnode *parent = x->parent;
	assert(parent != NULL);
	if (parent->left == x)
		x_btnode_rotate_right(x);
	else
		x_btnode_rotate_left(x);
}

static void mark_gp(x_btnode *x)
{
	assert(x->parent);
	x_btnode *parent = x->parent;
	x_btnode *grand = parent->parent;
	x->parent = grand;
	parent->parent = x;
	if (!grand)
		return;
	if (grand->left == parent)
		grand->left = x;
	else
		grand->right = x;
}

void x_btnode_insert_before(x_btnode *x, x_btnode *y)
{
	assert(y);
	x_btnode *node = y->left;
	x->left = x->right = NULL;
	if (!node) {
		y->left = x;
		x->parent = y;
		return;
	}
	while (node->right)
		node = node->right;
	node->right = x;
	x->left = x->right = NULL;
	x->parent = node;
}

void x_btnode_insert_after(x_btnode *x, x_btnode *y)
{
	assert(y);
	x_btnode *node = y->right;
	x->left = x->right = NULL;
	if (!node) {
		y->right = x;
		x->parent = y;
		return;
	}
	while (node->left)
		node = node->left;
	node->left = x;
	x->left = x->right = NULL;
	x->parent = node;
}

