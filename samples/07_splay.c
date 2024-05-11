#include "x/def.h"
#include "x/splay.h"
#include "x/flowctl.h"
#include <stdio.h>
#include <stdlib.h>

struct number { x_splay_node node; int i; };

int cmp(const x_splay_node *n1, const x_splay_node *n2)
{
	struct number *v1 = x_container_of(n1, struct number, node);
	struct number *v2 = x_container_of(n2, struct number, node);
	return v1->i - v2->i;
}

int main()
{
	x_splay t;
	x_splay_init(&t, cmp);
	
	x_forvalues(int, 7, 8, 3, 9, 4, 1, 2, 6, 5, 0) {
                struct number *p = malloc(sizeof *p);
                p->i = _;
                x_splay_insert(&t, &p->node);
        }

        x_splay_node *cur;

	cur = x_splay_first(&t);
        while (cur) {
                struct number *p = x_container_of(cur, struct number, node);
		printf("%d ", p->i);
                cur = x_splay_next(cur);
        }
	putchar('\n');

	cur = x_splay_last(&t);
        while (cur) {
                struct number *p = x_container_of(cur, struct number, node);
		printf("%d ", p->i);
                cur = x_splay_prev(cur);
        }
	putchar('\n');

	for (int i = 0; i < 10; i++) {
		struct number key = { .i = i };
                struct number *p = x_container_of(x_splay_find(&t, &key.node), struct number, node);
		printf("%d ", p->i);
	}
	putchar('\n');
}
