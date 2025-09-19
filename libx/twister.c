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
#include "x/twister.h"

#define UPPER_MASK 0x80000000
#define LOWER_MASK 0x7fffffff
#define TEMPERING_MASK_B 0x9d2c5680
#define TEMPERING_MASK_C 0xefc60000
#define STATE_VECTOR_M 397

void x_mt19937_init(x_mt19937* mt, uint32_t seed)
{
	/* set initial seeds to mt[X_MT19937_STATE_MAXLEN] using the generator
	 * from Line 25 of Table 1 in: Donald Knuth, "The Art of Computer
	 * Programming," Vol. 2 (2nd Ed.) pp.102.
	 */
	mt->mt[0] = seed & 0xffffffff;
	for (mt->index=1; mt->index<X_MT19937_STATE_MAXLEN; mt->index++)
		mt->mt[mt->index] = (6069 * mt->mt[mt->index - 1]) & 0xffffffff;
}

uint32_t x_mt19937_next(x_mt19937* mt)
{
	uint32_t y;
	const static uint32_t mag[2] = {0x0, 0x9908b0df}; /* mag[x] = x * 0x9908b0df for x = 0,1 */
	if(mt->index >= X_MT19937_STATE_MAXLEN || mt->index < 0) {
		/* generate X_MT19937_STATE_MAXLEN words at a time */
		int32_t kk;
		if (mt->index >= X_MT19937_STATE_MAXLEN + 1 || mt->index < 0)
			x_mt19937_init(mt, 4357);
		for(kk=0; kk<X_MT19937_STATE_MAXLEN-STATE_VECTOR_M; kk++) {
			y = (mt->mt[kk] & UPPER_MASK) | (mt->mt[kk+1] & LOWER_MASK);
			mt->mt[kk] = mt->mt[kk+STATE_VECTOR_M] ^ (y >> 1) ^ mag[y & 0x1];
		}
		for(; kk<X_MT19937_STATE_MAXLEN-1; kk++) {
			y = (mt->mt[kk] & UPPER_MASK) | (mt->mt[kk+1] & LOWER_MASK);
			mt->mt[kk] = mt->mt[kk+(STATE_VECTOR_M-X_MT19937_STATE_MAXLEN)] ^ (y >> 1) ^ mag[y & 0x1];
		}
		y = (mt->mt[X_MT19937_STATE_MAXLEN-1] & UPPER_MASK) | (mt->mt[0] & LOWER_MASK);
		mt->mt[X_MT19937_STATE_MAXLEN-1] = mt->mt[STATE_VECTOR_M-1] ^ (y >> 1) ^ mag[y & 0x1];
		mt->index = 0;
	}
	y = mt->mt[mt->index++];
	y ^= (y >> 11);
	y ^= (y << 7) & TEMPERING_MASK_B;
	y ^= (y << 15) & TEMPERING_MASK_C;
	y ^= (y >> 18);
	return y;
}

