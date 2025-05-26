#include "x/rope.h"
#include <stdio.h>
#include <stdlib.h>

static void dump(x_rope *r, int line)
{
	printf("dump:%d: len = %zd: ", line, x_rope_length(r));
	x_rope_dump_seq(r, stderr);
}

#define dump(r) dump(r, __LINE__)

int main()
{
	puts("[Build r1 and display]");
	x_rope r1;
	x_rope_init(&r1, "What");
	x_rope_append(&r1, "language");
	x_rope_append(&r1, "is");
	x_rope_append(&r1, "thine,");
	x_rope_append(&r1, "O sea?");
	x_rope_append(&r1, "The");
	x_rope_append(&r1, "language");
	x_rope_append(&r1, "of");
	x_rope_append(&r1, "eternal question");
	x_rope_dump_tree(&r1, stderr);

	puts("[Balance r1]");
	x_rope_balance(&r1);
	x_rope_dump_tree(&r1, stderr);


	puts("[Build r2]");
	x_rope r2;
	x_rope_init(&r2, NULL);
	x_rope_append(&r2, "What");
	x_rope_append(&r2, "language");
	x_rope_append(&r2, "is");
	x_rope_append(&r2, "thy");
	x_rope_append(&r2, "answer,");
	x_rope_append(&r2, "O sky?");
	x_rope_append(&r2, "The");
	x_rope_append(&r2, "language" );
	x_rope_append(&r2, "of");
	x_rope_append(&r2, "eternal silence.");
	x_rope_dump_tree(&r2, stdout);

	puts("[Merge r2 to r1]");
	x_rope_merge(&r1, &r2);
	x_rope_dump_tree(&r1, stdout);

	puts("[Splice r1]");
	char *data = x_rope_splice(&r1);
	printf("data = %s\n", data);
	free(data);

	puts("[Enumerate elements of r1]");
	x_rope_foreach(n, &r1) {
		printf("%s ", n->ptr);;
	}
	putchar('\n');

	puts("[Split r1 to r2 by offset 55]");
	x_rope_split(&r1, 55, &r2);
	x_rope_dump_tree(&r1, stdout);
	x_rope_dump_tree(&r2, stdout);

	return 0;
}
