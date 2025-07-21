#include "x/trick.h"
#include "x/heap.h"
#include <stdio.h>
#include <stdlib.h>

static bool less_then(const void *p1, const void *p2)
{
	return *(int *)p1 < *(int *)p2;
}

int main(void)
{
	int arr[100];

	x_heap h;
	x_heap_init(&h, less_then);

	{
		printf("origin: ");
		for (int i = 0; i < 100; i++) {
			arr[i] = rand() % 100;
			x_heap_push(&h, arr + i); 
			printf("%d ", arr[i]);
		}
		putchar('\n');
	}

	{
		int *top;
		printf("sorted: ");
		while ((top = x_heap_pop(&h)))
			printf("%d ", *top);
		putchar('\n');
	}

	x_heap_free(&h);
}

