#include "x/heap.h"
#include <stdio.h>
#include <stdlib.h>

static bool cmp(const void *p1, const void *p2, void *ctx)
{
	return *(int *)p1 < *(int *)p2;
}

int main()
{
	x_heap h;
	x_heap_init(&h, sizeof(int), 1, cmp, NULL);

	for (int i = 0; i < 100; i++) {
		int rnd = rand() % 100;
		x_heap_push(&h, &rnd); 
	}

	const int *top;
	while ((top = x_heap_top(&h))) {
		printf("%d ", *top);
		x_heap_pop(&h);
	}
	putchar('\n');

	x_heap_free(&h);
}

