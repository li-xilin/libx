#include "x/bitmap.h"
#include <stdio.h>
#include <stdlib.h>

static void print(x_bitmap *bm)
{
	for (int i = 0; i < x_bitmap_nbits(bm); i++)
		printf("%d ", x_bitmap_get(bm, i));
	putchar('\n');
}

int main()
{
	x_bitmap bm;
	char buf[4]; // 32 bits
	x_bitmap_init(&bm, buf, sizeof buf);

	x_bitmap_clear(&bm, 1);
	print(&bm);

	for (int i = 0; i < x_bitmap_nbits(&bm); i += 2)
		x_bitmap_set(&bm, i, 0);
	print(&bm);

	for (int i = 0; i < x_bitmap_nbits(&bm); i += 1)
		x_bitmap_toggle(&bm, i);
	print(&bm);

	printf("The num of set bits: %zu\n", x_bitmap_count(&bm));
}

