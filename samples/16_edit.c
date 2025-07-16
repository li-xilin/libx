#include "x/edit.h"
#include "x/file.h"
#include "x/string.h"
#include <stdlib.h>

int main(void)
{
	x_uchar *str = NULL;
	x_printf(x_u("Press CTRL+C to exit\n"));
	while (1) {
		free(str);
		str = x_edit_readline("Input> ");
		if (!str)
			continue;
		if (x_ustrcmp(str, "exit") == 0)
			break;
		if (str[0]) {
			x_printf("%s\n", str);
			x_edit_history_add(str);
		}
	}
}
