#include "x/flowctl.h"
#include <stdio.h>

int main(void)
{
	/* Like BASIC language for next statement.
	 * FOR i = a to b STEP s
	 *     ...
	 * NEXT i
	 */
	size_t sum;

	sum = 0;
	x_forrange(1, 101)
		sum += _;
	printf("1 + ... + 100 = %zu\n", sum);

	sum = 0;
	x_forrange(10, 1, -2)
		sum += _;
	printf("10 + 8 + 6 + ... + 2 = %zu\n", sum);
}

