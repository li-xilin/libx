#include "x/twister.h"
#include <stdio.h>
#include <time.h>

int main(void)
{
	x_mt19937 rand_ctx;
	x_mt19937_init(&rand_ctx, 1);
	for (int i = 0; i < 32; i++) {
		for (int j = 0; j < 8; j++) {
			printf("%12u", x_mt19937_next(&rand_ctx));
		}
		putchar('\n');
	}
}
