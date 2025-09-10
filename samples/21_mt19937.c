#include "x/random.h"
#include <stdio.h>

int main(void)
{
	x_mt19937_ctx rand_ctx;
	x_mt19937_ctx_init(&rand_ctx, 0xFF);

	for (int i = 0; i < 1000; i++) {
		printf("%u\n", x_mt19937_next(&rand_ctx));
	}
}
