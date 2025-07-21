#include "x/trick.h"
#include "x/heap.h"
#include "x/macros.h"
#include <stdio.h>
#include <time.h>
#include <stdlib.h>

struct foo {
	x_ranode node;
	int num;
};

static bool less_then(const x_ranode *p1, const x_ranode *p2)
{
	struct foo *f1 = x_container_of(p1, struct foo, node);
	struct foo *f2 = x_container_of(p2, struct foo, node);
	return f1->num < f2->num;
}

int main(void)
{
#define N 50
	struct foo arr[N];
	x_heap h;
	x_heap_init(&h, less_then);

	{
		printf("origin: ");
		for (int i = 0; i < N; i++) {
			arr[i].num = rand() % 100;
			x_heap_push(&h, &arr[i].node); 
			printf("%02d ", arr[i].num);
		}
		putchar('\n');
	}

	{
		printf("hindex: ");
		for (int i = 0; i < N; i++) {
			printf("%02zd ", arr[i].node.index);
		}
		putchar('\n');
	}

	{
		printf("remove: ");
		for (int i = 0; i < N / 2; i++) {
			printf("%02d ", arr[i].num);
			fflush(stdout);
			x_heap_remove(&h, &arr[i].node);
		}
		putchar('\n');
	}

	{
		printf("sorted: ");
		for (x_ranode *top; (top = x_heap_pop(&h)); ) {
			struct foo *f = x_container_of(top, struct foo, node);
			printf("%02d ", f->num);
		}
		putchar('\n');
	}

	x_heap_free(&h);
}

