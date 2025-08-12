/*
 * Copyright (c) 2022-2025 Li Xilin <lixilin@gmx.com>
 * 
 * Permission is hereby granted, free of charge, to one person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include "x/lib.h"
#include "x/detect.h"
#include "x/macros.h"

#if defined(X_OS_WIN)
#include <windef.h>
#include <winbase.h>
#include <libloaderapi.h>
#include <errhandlingapi.h>
#include <shellapi.h>
#include <stdio.h>
#else
#include <dlfcn.h>
#endif


#if defined(X_OS_WIN)
static wchar_t last_error_message_win32[256];
static void x_lib_win32_seterror(void)
{
	DWORD errcode = GetLastError();
	if (!FormatMessageW(FORMAT_MESSAGE_IGNORE_INSERTS|FORMAT_MESSAGE_FROM_SYSTEM,
			NULL, errcode, MAKELANGID(LANG_ENGLISH, SUBLANG_DEFAULT),
			last_error_message_win32, sizeof(last_error_message_win32) / 2 - 1, NULL)) {
		swprintf(last_error_message_win32, x_arrlen(last_error_message_win32), L"unknown error %lu", errcode);
	}
}
#endif

x_lib *x_lib_open(const x_uchar* fname)
{
#if defined(X_OS_WIN)
	HMODULE h;
	int emd;
	emd = SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOOPENFILEERRORBOX);
	h = LoadLibraryW(fname);
	SetErrorMode(emd);
	if(!h) {
		x_lib_win32_seterror();
		return NULL;
	}
	last_error_message_win32[0] = 0;
	return (x_lib *)h;
#else
	return dlopen(fname, RTLD_NOW | RTLD_GLOBAL);
#endif
}

void *x_lib_symbol(x_lib *lib, const char *func)
{
#if defined(X_OS_WIN)
	void *ptr;
	*(FARPROC*)(&ptr) = GetProcAddress((HMODULE)lib, func);
	if(!ptr) {
		x_lib_win32_seterror();
		return NULL;
	}
	last_error_message_win32[0] = 0;
	return ptr;
#else
	return dlsym(lib, func);
#endif
}

int x_lib_close(x_lib *lib)
{
	if (!lib)
		return 0;
#if defined(X_OS_WIN)
	if (!FreeLibrary((HMODULE)lib)) {
		x_lib_win32_seterror();
		return -1;
	}
	last_error_message_win32[0] = 0;
	return 0;
#else
	return - !!dlclose(lib);
#endif
}

const x_uchar *x_lib_strerror(void)
{
#if defined(X_OS_WIN)
	if (last_error_message_win32[0])
		return last_error_message_win32;
	else
		return NULL;
#else
	return dlerror();
#endif
}
