#include "x/test.h"
#include "x/hash.h"
#include "x/hmap.h"
#include "x/memory.h"
#include "x/dump.h"
#include <unistd.h>
#include <stdio.h>
#include <time.h>

#define NODE_MAX (1024 * 1024)

struct node {
	x_hnode node;
	int key;
	int mark;
};

static bool node_equal(const x_hnode *node1, const x_hnode *node2)
{
	struct node *p1 = x_container_of(node1, struct node, node);
	struct node *p2 = x_container_of(node2, struct node, node);
	return p1->key == p2->key;
}

static void hmap_insert(ut_runner *r)
{
	x_hmap ht = X_HMAP_INIT(node_equal);
	struct node *nodes = x_malloc(NULL, sizeof(struct node) * NODE_MAX);
	for (int i = 0; i < NODE_MAX; i++) {
		nodes[i].key = i;
		nodes[i].node.hash = x_hash64_twang(i);
		nodes[i].mark = 0;
		x_hmap_find_or_insert(&ht, &nodes[i].node);
	}
	ut_assert(r, x_hmap_sanitize(&ht) == true);
	ut_assert_int_equal(r, NODE_MAX, ht.elem_cnt);

	for (int i = 0; i < NODE_MAX; i++) {
		x_hnode *res = x_hmap_find_or_insert(&ht, &nodes[i].node);
		struct node *p = x_container_of(res, struct node, node);
		ut_assert_uint_equal(r, 0, p->mark);
		p->mark = 1;
		ut_assert(r, p != NULL);
		ut_assert_uint_equal(r, p->key, i);
	}

	int count = 0;
	x_hmap_foreach(node, &ht) {
		struct node *p = x_container_of(node, struct node, node);
		ut_assert_uint_equal(r, 1, p->mark);
		count++;
	}
	ut_assert_int_equal(r, NODE_MAX, count);
}

static void hmap_remove(ut_runner *r)
{
	x_hmap ht = X_HMAP_INIT(node_equal);
	struct node *nodes = x_malloc(NULL, sizeof(struct node) * NODE_MAX);
	for (int i = 0; i < NODE_MAX; i++) {
		nodes[i].key = i;
		nodes[i].node.hash = x_hash64_twang(i);
		nodes[i].mark = 0;
		x_hmap_find_or_insert(&ht, &nodes[i].node);
	}

	for (int i = 0; i < NODE_MAX / 2; i++) {
		x_hnode *hnode = x_hmap_find_and_remove(&ht, &nodes[i].node);
		ut_assert(r, hnode != NULL);
		struct node *p = x_container_of(hnode, struct node, node);
		p->mark = 1;
	}
	for (int i = 0; i < NODE_MAX / 2; i++)
		ut_assert_uint_equal(r, 1, nodes[i].mark);
	for (int i = NODE_MAX / 2; i < NODE_MAX; i++)
		ut_assert_uint_equal(r, 0, nodes[i].mark);

	for (int i = NODE_MAX / 2; i < NODE_MAX; i++)
		x_hmap_remove(&ht, &nodes[i].node);
	ut_assert_int_equal(r, 0, ht.elem_cnt);
}

void hmap_test_init(ut_suite *s)
{
	ut_suite_init(s, "sklist.h");
	ut_suite_add(s, hmap_insert);
	ut_suite_add(s, hmap_remove);
}
