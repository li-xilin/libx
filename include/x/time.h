/*
 * Copyright (c) 2022-2023,2025 Li Xilin <lixilin@gmx.com>
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

#ifndef X_TIME_H
#define X_TIME_H

#include "detect.h"
#ifdef X_OS_WIN
#include <sysinfoapi.h>
#include <winsock2.h>
#else
#include <sys/time.h>
#endif

#define x_time_diff(tv1, tv2, tv_out) \
	do { \
		(tv_out)->tv_sec = (tv1).tv_sec - (tv2).tv_sec; \
		(tv_out)->tv_usec = (tv1).tv_usec - (tv2).tv_usec; \
		if ((tv_out)->tv_usec < 0) { \
			(tv_out)->tv_usec += 1000000; \
			(tv_out)->tv_sec--; \
		} \
	} while(0)
#define x_time_forward(tv, msec) \
	do { \
		(tv).tv_sec += (msec) / 1000; \
		(tv).tv_usec += ((msec) % 1000) * 1000; \
		if ((tv).tv_usec > 1000000){ \
			(tv).tv_usec -= 1000000; \
			(tv).tv_sec++; \
		} \
	} while(0)
#define x_time_rewind(tv, msec) \
	do { \
		(tv).tv_sec -= (msec) / 1000; \
		(tv).tv_usec -= ((msec) % 1000) * 1000; \
		if ((tv).tv_usec < 0){ \
			(tv).tv_usec += 1000000; \
			(tv).tv_sec--; \
		} \
	} while(0)
#define x_time_set_msec(tv, msec) \
	do { \
		(tv).tv_sec = (msec) / 1000; \
		(tv).tv_usec = ((msec) % 1000) * 1000; \
	} while(0)
#define x_time_to_msec(tv) \
	((tv).tv_sec * 1000 + (tv).tv_usec / 1000)
#define x_time_ge(t1, t2) \
	((t1).tv_sec > (t2).tv_sec || ((t1).tv_sec == (t2).tv_sec && (t1).tv_usec >= (t2).tv_usec))
#define x_time_le(t1, t2) \
	((t1).tv_sec < (t2).tv_sec || ((t1).tv_sec == (t2).tv_sec && (t1).tv_usec <= (t2).tv_usec))
#define x_time_gt(t1, t2) \
	((t1).tv_sec > (t2).tv_sec || ((t1).tv_sec == (t2).tv_sec && (t1).tv_usec > (t2).tv_usec))
#define x_time_lt(t1, t2) \
	((t1).tv_sec < (t2).tv_sec || ((t1).tv_sec == (t2).tv_sec && (t1).tv_usec < (t2).tv_usec))

int x_time_now(struct timeval *tv);

#endif

