#include "x/rope.h"
#include "x/splay.h"
#include "x/string.h"
#include <string.h>
#include <assert.h>
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>

static x_rope_node *find_node(x_rope_node *root, size_t *index);
// static x_rope_node *insert_data(x_rope_node *x, size_t index, char *data, size_t len);
static x_rope_node *split_node(x_rope_node *x, size_t index);
static x_rope_node *clone_node(const x_rope_node *x);
static void splay_node(x_rope_node *x);
static void eval_weight(x_rope_node *x);

#define BALANCE_BLOCK_SIZE 4096
#define BALANCE_BLOCK_NNODES (BALANCE_BLOCK_SIZE / sizeof(uintptr_t))

int x_rope_init(x_rope *r, const char *str)
{
	if (!str) {
		r->root_node = NULL;
		return 0;
	}
	size_t size;
	char *dup = x_strdup2(str, &size);
	if (!dup)
		return -1;
	r->root_node = calloc(1, sizeof(x_rope_node));
	r->root_node->ptr = dup;
	r->root_node->size = size;
	r->root_node->weight = size;
	return 0;
}

int x_rope_init1(x_rope *r, char *ptr, size_t len)
{
	if (!ptr) {
		r->root_node = NULL;
		return 0;
	}
	r->root_node = calloc(1, sizeof(x_rope_node));
	if (!r->root_node)
		return -1;
	r->root_node->ptr = ptr;
	r->root_node->size = len;
	r->root_node->weight = len;
	return 0;
}

void x_rope_free(x_rope *r)
{
	x_rope n;
	if (!r->root_node)
		return;
	n.root_node = x_container_of(r->root_node->node.left, x_rope_node, node);
	x_rope_free(&n);
	n.root_node = x_container_of(r->root_node->node.right, x_rope_node, node);
	x_rope_free(&n);
	n.root_node = x_container_of(r->root_node, x_rope_node, node);
	free(n.root_node->ptr);
	free(n.root_node);
	r->root_node = NULL;
}

const x_rope_node *x_rope_get_node(x_rope *r, size_t index)
{
	x_rope_node *n = find_node(r->root_node, &index);
	if (!n)
		return NULL;
	splay_node(n);
	r->root_node = n;
	return n;
}

void x_rope_swap(x_rope *r, x_rope *r1)
{
	x_rope_node *tmp = r->root_node;
	r->root_node = r1->root_node;
	r1->root_node = tmp;
}

static x_rope_node *find_node(x_rope_node *root, size_t *index)
{
	assert(root);
	x_btnode *curr = &root->node;
	x_rope_node *right, *node = x_container_of(curr, x_rope_node, node);
	x_rope_node *left = x_container_of(curr->left, x_rope_node, node);
	if (!curr || *index >= node->weight) {
		errno = ERANGE;
		return NULL;
	}
	size_t weight, left_weight = curr->left ? left->weight : 0;
	while (1) {
		left = x_container_of(curr->left, x_rope_node, node);
		right = x_container_of(curr->right, x_rope_node, node);
		weight = node->weight;
		left_weight = curr->left ? left->weight : 0;
		assert(*index < weight);
		if (*index < left_weight) {
			curr = curr->left;
			node = left;
		}
		else if (*index - left_weight < node->size) {
			*index -= left_weight;
			break;
		}
		else {
			*index -= left_weight + node->size;
			assert(curr->right && *index < right->weight);
			curr = curr->right;
			node = right;
		}
	}
	return node;
}

#if 0
static x_rope_node *insert_data(x_rope_node *x, size_t index, char *data, size_t len)
{
	x_rope_node *new = NULL;
	char *new_ptr = NULL;
	assert(x);
	assert(index < x->size);

	if (x->size < X_ROPE_SPLIT_SIZE) {
		new_ptr = realloc(x->ptr, x->size + len + 1);
		if (!new_ptr)
			return NULL;
		memmove(x->ptr + index + len, x->ptr + index, x->size - index);
		memcpy(x->ptr + index, data, len);
		x->ptr = new_ptr;
		x->size = x->size + len;
		return x;
	}
		
	new = malloc(sizeof *new);
	if (!new)
		return NULL;

	x_btnode_insert_after(&new->node, &x->node);

	new->size = x->size - index + len;
	new->ptr = malloc(new->size + 1);
	if (!new->ptr) {
		free(new);
		return NULL;
	}
	memcpy(new->ptr, x->ptr + index, x->size - index);
	memcpy(new->ptr + x->size - index, data, len);
	new->ptr[new->size] = '\0';

	new_ptr = realloc(x->ptr, index + 1);
	if (!new_ptr) {
		free(new->ptr);
		free(new);
	}
	new_ptr[index] = '\0';
	x->size = index;
	x->ptr = new_ptr;
	return new;
}
#endif

static x_rope_node *split_node(x_rope_node *x, size_t index)
{
	x_rope_node *new = NULL;
	char *new_ptr = NULL;
	assert(x);
	assert(index < x->size);
	if (index == 0) {
		return x;
	}
	new = malloc(sizeof *new);
	if (!new)
		return NULL;
	new->size = x->size - index;
	new->ptr = malloc(new->size + 1);
	if (!new->ptr) {
		free(new);
		return NULL;
	}
	memcpy(new->ptr, x->ptr + index, new->size);
	new->ptr[new->size] = '\0';
	x_btnode_insert_after(&new->node, &x->node);
	eval_weight(new);

	new_ptr = realloc(x->ptr, index + 1);
	if (!new_ptr) {
		free(new->ptr);
		free(new);
	}
	new_ptr[index] = '\0';
	x->size = index;
	x->ptr = new_ptr;
	eval_weight(x);
	return new;
}

static x_rope_node *clone_node(const x_rope_node *x)
{
	x_rope_node *new_child = NULL, *new = NULL;
	assert(x);
	new = malloc(sizeof *new);
	if (!new)
		goto err;
	memset(&new->node, 0, sizeof new->node);
	new->weight = x->weight;
	new->size = x->size;
	new->ptr = x_memdup(x->ptr, x->size);
	if (!new->ptr) {
		free(new);
		goto err;
	}
	if (x->node.left) {
		new_child = clone_node(x_container_of(x->node.left, x_rope_node, node));
		if (!new_child)
			goto err;
		new->node.left = &new_child->node;
		new_child->node.parent = &new->node;
	}
	if (x->node.right) {
		new_child = clone_node(x_container_of(x->node.right, x_rope_node, node));
		if (!new_child)
			goto err;
		new->node.right = &new_child->node;
		new_child->node.parent = &new->node;
	}
	return new;
err:
	free(new);
	return NULL;
}

static void splay_node(x_rope_node *n)
{
	x_btnode *x = &n->node;
	while (1) {
		x_btnode *p = x->parent;
		if (!p)
			break;
		x_btnode *g = p->parent;
		if (!p->parent) {
			x_btnode_zig(x);
			eval_weight(x_container_of(p, x_rope_node, node));
			continue;
		}
		if (x == p->left && p == g->left) {
			x_btnode_zigzig(x, p);
			eval_weight(x_container_of(g, x_rope_node, node));
			eval_weight(x_container_of(p, x_rope_node, node));
			continue;
		}
		if (x == p->right && p == g->right) {
			x_btnode_zigzig(x, p);
			eval_weight(x_container_of(g, x_rope_node, node));
			eval_weight(x_container_of(p, x_rope_node, node));
			continue;
		}
		x_btnode_zigzag(x, p);
		eval_weight(x_container_of(p, x_rope_node, node));
		eval_weight(x_container_of(g, x_rope_node, node));
	}
	eval_weight(n);
}

static void eval_weight(x_rope_node *x)
{
	size_t weight = x->size;
	if (x->node.left)
		weight += x_container_of(x->node.left, x_rope_node, node)->weight;
	if (x->node.right)
		weight += x_container_of(x->node.right, x_rope_node, node)->weight;
	x->weight = weight;
}

int x_rope_append(x_rope *r, const char *str)
{
	x_rope tmp;
	x_rope_init(&tmp, str);
	return x_rope_insert(r, x_rope_length(r), &tmp);
}

int x_rope_clone(const x_rope *r, x_rope *out)
{
	if (!r->root_node) {
		out->root_node = NULL;
		return 0;
	}

	x_rope_node *root = clone_node(r->root_node);
	if (!root)
		return -1;
	out->root_node = root;
	root->node.parent = NULL;
	return 0;
}

int x_rope_split(x_rope *r, size_t index, x_rope *tail)
{
	if (index == x_rope_length(r)) {
		tail->root_node = NULL;
		return 0;
	}
	x_rope_node *node = find_node(r->root_node, &index);
	if (!node)
		return -1;
	x_rope_node *mid = split_node(node, index);
	splay_node(mid);
	r->root_node = x_container_of(mid->node.left, x_rope_node, node);
	tail->root_node = mid;
	if (mid->node.left) {
		mid->node.left->parent = NULL;
		mid->node.left = NULL;
		eval_weight(mid);
	}
	return 0;
}



void x_rope_merge(x_rope *dst, x_rope *src)
{
	if (!src->root_node)
		return;
	if (!dst->root_node) {
		dst->root_node = src->root_node;
		src->root_node = NULL;;
		return;
	}

	x_rope_node *mid = x_container_of(x_btnode_last(&dst->root_node->node), x_rope_node, node);
	splay_node(mid);
	mid->node.right = &src->root_node->node;
	src->root_node->node.parent = &mid->node;
	eval_weight(mid);
	dst->root_node = mid;
	src->root_node = NULL;
}

int x_rope_insert(x_rope *r, size_t index, x_rope *ins)
{
	if (x_rope_length(r) == index) {
		x_rope_merge(r, ins);
		return 0;
	}
	x_rope_node *node = find_node(r->root_node, &index);
	if (!node)
		return -1;
	x_rope tmp;
	if (x_rope_split(r, index, &tmp))
		return -1;

	x_rope_merge(r, ins);
	x_rope_merge(r, &tmp);
	return 0;
}

int x_rope_remove(x_rope *r, const size_t index, size_t length, x_rope *out)
{
	int err;
	x_rope tail, del;
	if (index + length > r->root_node->weight) {
		errno = ERANGE;
		return -1;
	}
	if (length == 0)
		return 0;
	err = x_rope_split(r, index + length, &tail);
	assert(err == 0);
	err = x_rope_split(r, index, &del);
	assert(err == 0);
	x_rope_merge(r, &tail);
	if (out)
		*out = del;
	else
		x_rope_free(&del);
	return 0;
}

size_t x_rope_length(const x_rope *r)
{
	return r->root_node ? r->root_node->weight : 0;
}

static void dump_ptr(const x_rope_node *n, int max, FILE *fp)
{
	if (max < 0)
		max = n->size;
	size_t outlen = x_min(n->size, max);
	for (int i = 0; i < outlen; i++) {
		int c = n->ptr[i];
		if (isprint(c))
			fputc(c, fp);
		else if (c == '\t')
			fputs("\\t", fp);
		else if (c == '\n')
			fputs("\\n", fp);
		else if (c == '\r')
			fputs("\\r", fp);
		else if (c == '\b')
			fputs("\\b", fp);
		else
			fprintf(fp, "\\x%02x", c);
	}
	if (n->size > max)
		fputs("...", fp);
}

void x_rope_dump_seq(const x_rope *r, void *fp)
{
	if (!r->root_node) {
		putchar('\n');
		return;
	}
	x_btnode *cur = x_btnode_first(&r->root_node->node);
	int i = 0;
	while (cur) {
		fprintf(fp, "%d ", i++);
		dump_ptr(x_container_of(cur, x_rope_node, node), -1, fp);
		fputc('\n', fp);
		cur = x_btnode_next(cur);
	}
}

static void print_tree(x_rope_node *n, char *prefix, size_t len, bool is_left, bool has_right, FILE *fp)
{
#define PREFIX_LEFT "├── "
#define PREFIX_RIGHT "└── "
#define PREFIX_LINE "│   "
#define PREFIX_NOLINE "    "
	int prefix_inc_len = 0;
    if (!n)
		return;
	fprintf(fp, "%-9zu %s", n->weight, prefix);
	if (n->node.parent) {
		fprintf(fp, "%s", is_left ? PREFIX_LEFT : PREFIX_RIGHT);
		dump_ptr(n, 50, fp);
		fputc('\n', fp);
		if (has_right) {
			strcpy(prefix + len, PREFIX_LINE);
			prefix_inc_len = sizeof PREFIX_LINE - 1;
		}
		else {
			strcpy(prefix + len, PREFIX_NOLINE);
			prefix_inc_len = sizeof PREFIX_NOLINE - 1;
		}
	}
	else {
		fprintf(fp, "%s\n", n->ptr);
	}
	x_rope_node *left = x_container_of(n->node.left, x_rope_node, node);
	x_rope_node *right = x_container_of(n->node.right, x_rope_node, node);
    print_tree(right, prefix, len + prefix_inc_len, true, n->node.left, fp);
    print_tree(left, prefix, len + prefix_inc_len, false, false, fp);
	prefix[len] = '\0';
}

void x_rope_dump_tree(const x_rope *r, void *fp)
{
	char prefix[512];
	if (!r->root_node) {
		fputs("empty\n", fp);
		return;
	}
	prefix[0] = '\0';
	fprintf(fp, "%-9s %s\n", "Weight", "Root");
	print_tree(r->root_node, prefix, 0, false, false, fp);
	putchar('\n');
}

char *x_rope_at(const x_rope *r, size_t index)
{
	x_rope_node *node = find_node(r->root_node, &index);
	if (!node)
		return NULL;
	return node->ptr + index;
}

char *x_rope_splice(const x_rope *r)
{
	size_t offset = 0;
	char *buf = malloc(x_rope_length(r) + 1);
	if (!buf)
		return NULL;
	x_btnode *cur = x_btnode_first(&r->root_node->node);
	while (cur) {
		x_rope_node *n = x_container_of(cur, x_rope_node, node);
		cur = x_btnode_next(cur);
		assert(offset + n->size <= x_rope_length(r));
		memcpy(buf + offset, n->ptr, n->size);
		offset += n->size;
	}
	assert(offset == x_rope_length(r));
	buf[offset] = '\0';
	return buf;
}

int x_rope_vprintf(x_rope *r, size_t index, const char *fmt, va_list ap)
{
	va_list ap1;
	va_copy(ap1, ap);
	int len = vsnprintf(NULL, 0, fmt, ap);
	if (len < 0) {
		va_end(ap1);
		return -1;
	}
	char *buf = malloc(len + 1);
	if (!buf) {
		va_end(ap1);
		return -1;
	}
	vsprintf(buf, fmt, ap1);
	va_end(ap1);
	buf[len] = '\0';

	x_rope tail;
	if (x_rope_init1(&tail, buf, len)) {
		free(buf);
		return -1;
	}

	if (index == x_rope_length(r)) {
		x_rope_merge(r, &tail);
		return 0;
	}
	return x_rope_insert(r, index, &tail);
}

int x_rope_printf(x_rope *r, size_t index, const char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	int ret = x_rope_vprintf(r, index, fmt, ap);
	va_end(ap);
	return ret;
}

static x_rope_node *build_tree(x_rope_node ***table, long left, long right, x_rope_node* parent)
{
    if (left > right)
		return NULL;
	long mid = (left + right) / 2;
	x_rope_node *node = table[mid / BALANCE_BLOCK_NNODES][mid % BALANCE_BLOCK_NNODES];
	node->node.left = &build_tree(table, left, mid - 1, node)->node;
	node->node.right = &build_tree(table, mid + 1, right, node)->node;
	node->node.parent = &parent->node;
	eval_weight(node);
	return node;
}

int x_rope_balance(x_rope *r)
{
	int ret = -1;
	long nnodes = 0;
	x_rope_node ***node_tab = NULL;
	x_btnode *cur = x_btnode_first(&r->root_node->node);
	while (cur) {
		if (!(nnodes % BALANCE_BLOCK_NNODES)) {
			x_rope_node *** new_tab = realloc(node_tab, nnodes / BALANCE_BLOCK_NNODES);
			if (!new_tab)
				goto out;
			if (!(new_tab[nnodes / BALANCE_BLOCK_NNODES] = malloc(BALANCE_BLOCK_SIZE))) {
				free(new_tab);
				goto out;
			}
			node_tab = new_tab;
		}
		node_tab[nnodes / BALANCE_BLOCK_NNODES][nnodes % BALANCE_BLOCK_NNODES]
			= x_container_of(cur, x_rope_node, node);
		cur = x_btnode_next(cur);
		nnodes++;
	}
	r->root_node = build_tree(node_tab, 0, nnodes - 1, NULL);
	ret = 0;
out:
	for (int i = 0; i < ((nnodes - 1) / BALANCE_BLOCK_NNODES) + 1; i++)
		free(node_tab[i]);
	free(node_tab);
	return ret;
}

