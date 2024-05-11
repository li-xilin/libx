#ifndef AX_SPLAY_H
#define AX_SPLAY_H

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

typedef int splay_comp_f(const x_btnode *left, const x_btnode *right);

struct x_splay_st {
    x_btnode *root;
    splay_comp_f *comp;
    size_t size;
};

inline static bool splay_linked(x_splay *t, const x_btnode *node)
{
	return node->left || node->right || node->parent || t->root == node;
}

x_btnode *splay_find(x_splay *t, const x_btnode *node);

x_btnode *splay_find_or_insert(x_splay *t, x_btnode *new);

void splay_replace(x_splay *t, x_btnode *old, x_btnode *new);

x_btnode *splay_replace_or_insert(x_splay *t, x_btnode *new);

void splay_remove(x_splay *t, x_btnode *node);

x_btnode *splay_first(x_splay *t);

x_btnode *splay_next(x_btnode *node);

x_btnode *splay_prev(x_btnode *node);

x_btnode *splay_last(x_splay *t);

#endif
