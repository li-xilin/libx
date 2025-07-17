#include "x/errno.h"
#include "x/detect.h"
#include "x/file.h"
#include "x/string.h"
#include <stdlib.h>
#include <locale.h>
#ifdef X_OS_WIN
#include <errhandlingapi.h>
#include <winerror.h>
#endif

int main(void)
{
#ifdef X_OS_WIN
	_wsetlocale(LC_ALL, L"chs");
#else
	setlocale(LC_ALL, "zh_CN.UTF-8");
#endif
	x_setmode_utf8(stdout);
	errno = X_EPERM;
	x_printf(x_u("EPERM -> %s\n"), x_last_error(errno));
#if defined(X_OS_WIN)
	SetLastError(ERROR_MAGAZINE_NOT_PRESENT);
	x_eval_errno();
	x_printf(x_u("ERROR_MAGAZINE_NOT_PRESENT -> %s\n"), x_last_error(errno));
#elif defined(X_OS_LINUX)
	errno = EHWPOISON;
	x_eval_errno();
	x_printf(x_u("EHWPOISON -> %s\n"), x_last_error(errno));
#endif
}
