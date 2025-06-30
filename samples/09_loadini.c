#include <x/ini.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

static int err_cb(int line, int err, void *args)
{
	fprintf(stderr, "error: line = %u, err = %s\n", line, x_ini_strerror(err));
	return 0;
}

int main(void)
{
	FILE *fp = fopen("./example.ini", "r");
	if (!fp) {
		fprintf(stderr, "fopen: %s\n", strerror(errno));
		return 1;
	}
	x_ini *d = x_ini_load(fp, NULL, err_cb, NULL);

	puts("\n--- Find option ---\n");

	printf("Pizza:Ham -> %s\n", x_ini_get(d, "Pizza", "Ham"));
	printf("wine:grape -> %s\n", x_ini_get(d, "wine", "grape"));
	printf("wine:grape1 -> %s\n", x_ini_get_str(d, "wine:grape1", "default"));

	puts("\n--- Add option ---\n");

	x_ini_set(d, "Foo", "Bar", "42", "test comment");
	printf("Foo:Bar -> %s\n", x_ini_get(d, "Foo", "Bar"));

	puts("\n--- Dump out ini content with comments ---\n");

	x_ini_dump(d, stdout);

	puts("\n--- Dump out ini content without comments ---\n");

	x_ini_pure(d);
	x_ini_dump(d, stdout);

}
