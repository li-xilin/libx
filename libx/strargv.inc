/*
 * Copyright (c) 2025 Li Xilin <lixilin@gmx.com>
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

#include "x/assert.h"
#include <stdlib.h>

#ifdef WIDE_CHAR
#define STRARGV x_wcsargv
#define CHAR_TYPE wchar_t
#define STRCPY wcscpy
#define STRLEN wcslen
#define STR_PREFIX(s) L##s
#else
#define STRARGV x_strargv
#define CHAR_TYPE char
#define STRCPY strcpy
#define STRLEN strlen
#define STR_PREFIX(s) s
#endif
#include <stdlib.h>
#include <wchar.h>
#include <string.h>

CHAR_TYPE **STRARGV(const CHAR_TYPE *cmdline, int* count)
{
	assert(count !=  NULL);
	assert(cmdline !=  NULL);
	CHAR_TYPE **argv, *d;
	int argc, qcount, bcount;
	/* --- First count the arguments */
	argc = 1;
	const CHAR_TYPE *s = cmdline;
	/* The first argument, the executable path, follows special rules */
	if (*s == STR_PREFIX('"')) {
		/* The executable path ends at the next quote, no matter what */
		s++;
		while (*s)
			if (*s++ == STR_PREFIX('"'))
				break;
	}
	else {
		/* The executable path ends at the next space, no matter what */
		while (*s && *s != STR_PREFIX(' ') && *s != STR_PREFIX('\t'))
			s++;
	}
	/* skip to the first argument, if any */
	while (*s == STR_PREFIX(' ') || *s == STR_PREFIX('\t'))
		s++;
	if (*s)
		argc++;

	/* Analyze the remaining arguments */
	qcount = bcount = 0;
	while (*s) {
		if ((*s == STR_PREFIX(' ') || *s == STR_PREFIX('\t')) && qcount == 0) {
			/* skip to the next argument and count it if any */
			while (*s == STR_PREFIX(' ') || *s == STR_PREFIX('\t'))
				s++;
			if (*s)
				argc++;
			bcount = 0;
		}
		else if (*s == STR_PREFIX('\\')) {
			/* STR_PREFIX('\'), count them */
			bcount++;
			s++;
		}
		else if (*s == STR_PREFIX('"')) {
			/* STR_PREFIX('"') */
			if ((bcount & 1) == 0)
				qcount++; /* unescaped STR_PREFIX('"') */
			s++;
			bcount = 0;
			/* consecutive quotes, see comment in copying code below */
			while (*s == STR_PREFIX('"')) {
				qcount++;
				s++;
			}
			qcount = qcount % 3;
			if (qcount == 2)
				qcount = 0;
		}
		else {
			/* a regular CHAR_TYPEacter */
			bcount = 0;
			s++;
		}
	}

	/* Allocate in a single lump, the string array, and the strings that go
	 * with it. This way the caller can make a single LocalFree() call to free
	 * both, as per MSDN.
	 */
	argv = malloc((argc+1) * sizeof(CHAR_TYPE *) + (STRLEN(cmdline) + 1) * sizeof(CHAR_TYPE));
	if (!argv)
		return NULL;
	CHAR_TYPE *cmdline1 = (CHAR_TYPE *)(argv + argc + 1);
	STRCPY(cmdline1, cmdline);
	/* --- Then split and copy the arguments */
	argv[0] = d = cmdline1;
	argc = 1;
	/* The first argument, the executable path, follows special rules */
	if (*d == STR_PREFIX('"')) {
		/* The executable path ends at the next quote, no matter what */
		s = d+1;
		while (*s) {
			if (*s == STR_PREFIX('"')) {
				s++;
				break;
			}
			*d++ = *s++;
		}
	}
	else {
		/* The executable path ends at the next space, no matter what */
		while (*d && *d != STR_PREFIX(' ') && *d != STR_PREFIX('\t'))
			d++;
		s = d;
		if (*s)
			s++;
	}
	/* close the executable path */
	*d++ = 0;
	/* skip to the first argument and initialize it if any */
	while (*s == STR_PREFIX(' ') || *s == STR_PREFIX('\t'))
		s++;
	if (!*s) {
		/* There are no parameters so we are all done */
		argv[argc] = NULL;
		*count = argc;
		return argv;
	}
	/* Split and copy the remaining arguments */
	argv[argc++] = d;
	qcount = bcount = 0;
	while (*s) {
		if ((*s == STR_PREFIX(' ') || *s == STR_PREFIX('\t')) && qcount == 0) {
			/* close the argument */
			*d++ = 0;
			bcount = 0;

			/* skip to the next one and initialize it if any */
			do {
				s++;
			} while (*s == STR_PREFIX(' ') || *s == STR_PREFIX('\t'));
			if (*s)
				argv[argc++] = d;
		}
		else if (*s == STR_PREFIX('\\')) {
			*d++ = *s++;
			bcount++;
		}
		else if (*s == STR_PREFIX('"')) {
			if ((bcount & 1) == 0) {
				/* Preceded by an even number of STR_PREFIX('\'), this is half that
				 * number of STR_PREFIX('\'), plus a quote which we erase.
				 */
				d -= bcount/2;
				qcount++;
			}
			else {
				/* Preceded by an odd number of STR_PREFIX('\'), this is half that
				 * number of STR_PREFIX('\') followed by a STR_PREFIX('"')
				 */
				d = d - bcount / 2 - 1;
				*d++ = STR_PREFIX('"');
			}
			s++;
			bcount = 0;
			/* Now count the number of consecutive quotes. Note that qcount
			 * already takes into account the opening quote if any, as well as
			 * the quote that lead us here.
			 */
			while (*s == STR_PREFIX('"')) {
				if (++qcount == 3)
				{
					*d++ = STR_PREFIX('"');
					qcount = 0;
				}
				s++;
			}
			if (qcount == 2)
				qcount = 0;
		}
		else {
			/* a regular CHAR_TYPEacter */
			*d++ = *s++;
			bcount = 0;
		}
	}
	*d = STR_PREFIX('\0');
	argv[argc] = NULL;
	*count = argc;
	return argv;
}

#undef STRARGV
#undef CHAR_TYPE
#undef STRCPY
#undef STRLEN
#undef WIDE_CHAR
#undef STR_PREFIX
