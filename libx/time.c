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

#include "x/time.h"
#include "x/detect.h"
#include "x/errno.h"
#include <stdlib.h>
#include <time.h>
#include <stdio.h>

int x_time_now(struct timeval * tv)
{
#ifdef X_OS_WIN
	union {
		long long ns100;
		FILETIME ft;
	} now;
	GetSystemTimeAsFileTime (&now.ft);
	tv->tv_usec = (long) ((now.ns100 / 10LL) % 1000000LL);
	tv->tv_sec = (long) ((now.ns100 - 116444736000000000LL) / 10000000LL);
	return (0);
#else
	return gettimeofday(tv, NULL);
#endif
}

uint64_t x_time_tick(void)
{
#ifdef X_OS_WIN
	return GetTickCount64();
#else
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000ULL + (uint64_t)ts.tv_nsec / 1000000ULL;
#endif
}

int x_time_from_iso8601(const char *datetime, struct timeval *tv)
{
    struct tm tm = {0};
    int tz_hour = 0, tz_min = 0;
    char tz_sign = '+';
    int fields;
    fields = sscanf(datetime, "%d-%d-%dT%d:%d:%d%c%02d:%02d",
                   &tm.tm_year, &tm.tm_mon, &tm.tm_mday,
                   &tm.tm_hour, &tm.tm_min, &tm.tm_sec,
                   &tz_sign, &tz_hour, &tz_min);
    tm.tm_year -= 1900;
    tm.tm_mon -= 1;
    tm.tm_isdst = -1;
    int tz_offset = 0;
    if (fields == 9) {
        tz_offset = (tz_hour * 3600 + tz_min * 60);
        if (tz_sign == '-')
			tz_offset = -tz_offset;
    }
	else if (fields != 6 && fields != 7) {
		errno = X_EINVAL;
        return -1;
	}
    time_t local_time = mktime(&tm);
    if (local_time == (time_t)-1) {
		x_eval_errno();
		return -1;
	}
    struct tm gmt_tm;
#ifdef X_OS_WIN
    if (gmtime_s(&gmt_tm, &local_time)) {
		x_eval_errno();
		return -1;
	}
#else
    if (!gmtime_r(&local_time, &gmt_tm)) {
		x_eval_errno();
		return -1;
	}
#endif
    time_t gmt_time = mktime(&gmt_tm);
    time_t timezone_offset = local_time - gmt_time;
    time_t utc_time = local_time - timezone_offset;
    utc_time -= tz_offset;
	tv->tv_sec = utc_time;
	tv->tv_usec = 0;
	return 0;
}
