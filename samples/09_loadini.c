#include <x/ini.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

int err_cb(unsigned line, unsigned err, void *args)
{
	fprintf(stderr, "error: line = %u, err = %s\n", line, x_ini_strerror(err));
	return 0;
}
int main()
{
	FILE *fp = fopen("./example.ini", "r");
	if (!fp) {
		fprintf(stderr, "fopen: %s\n", strerror(errno));
		return 1;
	}
	x_ini *d = x_ini_load(fp, err_cb, NULL);

	puts("\nFind option:");
	char buf[1024];
	printf("Pizza:Ham -> %s\n", x_ini_get(d, "Pizza", "Ham"));

	printf("wine:grape -> %s\n", x_ini_get(d, "wine", "grape"));

	printf("wine:grape -> %s\n", x_ini_get_str(d, "wine:grape", "default"));

	puts("\nAdd option:");
	x_ini_set(d, "Foo", "Bar", "42", "test comment");
	printf("Foo:Bar -> %s\n", x_ini_get(d, "Foo", "Bar"));


	puts("\nDump example.ini:");

	x_ini_dump(d, stdout);

}
