#include "x/list.h"
#include "x/flowctl.h"
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char *argv[])
{
	struct item { x_link link; int n; };
	x_list l;
	x_list_init(&l);

	x_repeat(10) {
		struct item *x = malloc(sizeof *x);
		x->n = _;
		x_list_add_back(&l, &x->link);
	}

	x_list_foreach(cur, &l) {
		struct item *x = x_container_of(cur, struct item, link);
		printf("%d ", x->n);
	}
	putchar('\n');

	x_list_popeach(cur, &l) {
		struct item *x = x_container_of(cur, struct item, link);
		printf("%d ", x->n);
		free(x);
	}
	putchar('\n');
}
