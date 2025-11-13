#include "x/fwalker.h"
#include "x/file.h"
#include "x/uchar.h"
#include "x/errno.h"
#include "x/stat.h"
#include <stdlib.h>

int x_main(int argc, x_uchar *argv[])
{
	if (argc <= 1) {
		x_fprintf(stderr, x_u("error: Root path not specified\n"));
		exit(1);
	}
	x_fwalker fw;
	if (x_fwalker_open(&fw, argv[1])) {
		x_fprintf(stderr, x_u("failed to open directory (%S)\n"), x_last_error(errno));
		exit(1);
	}
	x_stat stat;
	x_uchar *path;
	while ((path = x_fwalker_read(&fw, &stat)))
		x_printf(x_u("%S\n"), path);
	return 0;
}
