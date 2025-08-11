#include "x/memory.h"

int main(void)
{
	x_mset mset;
	x_mset_init(&mset);

	char *table[1024];

	for (int i = 0; i < 1024; i++)
		table[i] = x_malloc(&mset, 16);

	for (int i = 0; i < 1024; i++)
		x_free(table[i]);
	
	x_mset_free(&mset);
}

