/*
 * Copyright (c) 2025 Li Xilin <lixilin@gmx.com>
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
#include "x/random.h"
#include "x/detect.h"
#include "x/errno.h"
#include <stdint.h>

#ifdef X_OS_WIN
#include "wincrypt.h"

int x_random(void *buf, size_t size)
{
	HCRYPTPROV l_prov;
	if (!CryptAcquireContext(&l_prov, NULL, NULL, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT)) {
		x_eval_errno();
		return -1;
	}
	CryptGenRandom(l_prov, size, buf);
	CryptReleaseContext(l_prov, 0);
	return 0;
}

#else
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#ifndef O_CLOEXEC
#define O_CLOEXEC 0
#endif

int x_random(void *buf, size_t size)
{
	int l_fd = open("/dev/urandom", O_RDONLY | O_CLOEXEC);
	if(l_fd == -1) {
		l_fd = open("/dev/random", O_RDONLY | O_CLOEXEC);
		if(l_fd == -1) {
			x_eval_errno();
			return -1;
		}
	}
	char *l_ptr = buf;
	size_t l_left = size;
	while(l_left > 0) {
		int l_read = read(l_fd, l_ptr, l_left);
		if(l_read <= 0) {
			close(l_fd);
			x_eval_errno();
			return -1;
		}
		l_left -= l_read;
		l_ptr += l_read;
	}
	close(l_fd);
	return 0;
}

#endif

