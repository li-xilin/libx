#include "x/edit.h"
#include "x/file.h"
#include "x/string.h"
#include "x/memory.h"
#include <stdlib.h>

int main(void)
{
	x_uchar *str = NULL;
	x_setmode_utf8(stdout);
	x_printf(x_u("type 'exit' to quit program\n"));
	while (1) {
		x_free(str);
		str = x_edit_readline(x_u("Input> "));
		if (!str)
			continue;
		if (x_ustrcmp(str, x_u("exit")) == 0)
			break;
		if (str[0]) {
			x_printf(x_u("%s\n"), str);
			x_edit_history_add(str);
		}
	}
}
