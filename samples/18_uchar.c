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
	_wsetlocale(LC_ALL, L".65001");
#else
	setlocale(LC_ALL, "zh_CN.UTF-8");
#endif
	x_setmode_utf8(stdout);
	
	uint8_t u8str[] = { 0xe4, 0xbd, 0xa0, 0xe5, 0xa5, 0xbd, 0xe4, 0xb8, 0x96, 0xe7, 0x95, 0x8c, 0};
	wchar_t u16str[] = { 0x4f60, 0x597d, 0x4e16, 0x754c, 0x0000 };
	x_printf(x_u("str = %S\n"), x_uconv(u8str));
	x_printf(x_u("str = %S\n"), x_uconv(u16str));
}

