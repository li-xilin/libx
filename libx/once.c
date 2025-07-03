/*
 * Copyright (c) 2022-2025 Li Xilin <lixilin@gmx.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
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

#include "x/once.h"
#include "x/errno.h"

#ifdef X_OS_WIN
static BOOL CALLBACK __x_once_proc_win32(PINIT_ONCE InitOnce, PVOID pInitProc, PVOID *lpContext)
{
	((x_once_fn *)(uintptr_t)pInitProc)();
	return TRUE;
}
#endif

int x_once_init(x_once *once, x_once_fn *once_proc)
{
#ifdef X_OS_WIN
	if (!InitOnceExecuteOnce(&once->InitOnce, __x_once_proc_win32, (PINIT_ONCE_FN *)(INT_PTR)once_proc, NULL)) {
		x_eval_errno();
		return -1;
	}
#else
	int err = pthread_once(&once->once, once_proc);
	if (err) {
		errno = err;
		x_eval_errno();
		return -1;
	}
#endif
	return 0;
}
