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
#include <stdint.h>

#define UPPER_MASK 0x80000000
#define LOWER_MASK 0x7fffffff
#define TEMPERING_MASK_B 0x9d2c5680
#define TEMPERING_MASK_C 0xefc60000
#define STATE_VECTOR_M 397

#ifdef X_OS_WIN

int x_random(void *buf, size_t size)
{
	HCRYPTPROV l_prov;
	if (!CryptAcquireContext(&l_prov, NULL, NULL, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT))
		return -1;
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
		if(l_fd == -1)
			return -1;
	}
	char *l_ptr = buf;
	size_t l_left = size;
	while(l_left > 0) {
		int l_read = read(l_fd, l_ptr, l_left);
		if(l_read <= 0) {
			close(l_fd);
			return -1;
		}
		l_left -= l_read;
		l_ptr += l_read;
	}
	close(l_fd);
	return 0;
}

#endif

void x_mt19937_ctx_init(x_mt19937_ctx* rand, uint32_t seed)
{
	/* set initial seeds to mt[X_MT19937_STATE_MAXLEN] using the generator
	 * from Line 25 of Table 1 in: Donald Knuth, "The Art of Computer
	 * Programming," Vol. 2 (2nd Ed.) pp.102.
	 */
	rand->mt[0] = seed & 0xffffffff;
	for (rand->index=1; rand->index<X_MT19937_STATE_MAXLEN; rand->index++)
		rand->mt[rand->index] = (6069 * rand->mt[rand->index - 1]) & 0xffffffff;
}

uint32_t x_mt19937_next(x_mt19937_ctx* rand)
{
	uint32_t y;
	const static uint32_t mag[2] = {0x0, 0x9908b0df}; /* mag[x] = x * 0x9908b0df for x = 0,1 */
	if(rand->index >= X_MT19937_STATE_MAXLEN || rand->index < 0) {
		/* generate X_MT19937_STATE_MAXLEN words at a time */
		int32_t kk;
		if (rand->index >= X_MT19937_STATE_MAXLEN + 1 || rand->index < 0) {
			x_mt19937_ctx_init(rand, 4357);
		}
		for(kk=0; kk<X_MT19937_STATE_MAXLEN-STATE_VECTOR_M; kk++) {
			y = (rand->mt[kk] & UPPER_MASK) | (rand->mt[kk+1] & LOWER_MASK);
			rand->mt[kk] = rand->mt[kk+STATE_VECTOR_M] ^ (y >> 1) ^ mag[y & 0x1];
		}
		for(; kk<X_MT19937_STATE_MAXLEN-1; kk++) {
			y = (rand->mt[kk] & UPPER_MASK) | (rand->mt[kk+1] & LOWER_MASK);
			rand->mt[kk] = rand->mt[kk+(STATE_VECTOR_M-X_MT19937_STATE_MAXLEN)] ^ (y >> 1) ^ mag[y & 0x1];
		}
		y = (rand->mt[X_MT19937_STATE_MAXLEN-1] & UPPER_MASK) | (rand->mt[0] & LOWER_MASK);
		rand->mt[X_MT19937_STATE_MAXLEN-1] = rand->mt[STATE_VECTOR_M-1] ^ (y >> 1) ^ mag[y & 0x1];
		rand->index = 0;
	}
	y = rand->mt[rand->index++];
	y ^= (y >> 11);
	y ^= (y << 7) & TEMPERING_MASK_B;
	y ^= (y << 15) & TEMPERING_MASK_C;
	y ^= (y >> 18);
	return y;
}

