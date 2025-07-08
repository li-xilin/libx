/*
 * Copyright (c) 2023 Li Xilin <lixilin@gmx.com>
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

#ifndef X_CLIARG_H
#define X_CLIARG_H

#include "uchar.h"
#include "types.h"

struct x_cliarg_st {
    x_uchar **argv;
    int permute;
    int optind;
    int optopt;
    x_uchar *optarg;
    x_uchar errmsg[64];
    int subopt;
};

struct x_cliarg_long_st {
    const x_uchar *longname;
    int shortname;
    enum {
		X_CLIARG_NONE,
		X_CLIARG_REQUIRED,
		X_CLIARG_OPTIONAL
	} argtype;
};

void x_cliarg_init(x_cliarg *cliarg, x_uchar **argv);

/**
 * Read the next option in the argv array.
 * @param optstring a getopt()-formatted option string.
 * @return the next option character, -1 for done, or '?' for error
 *
 * Just like getopt(), a character followed by no colons means no
 * argument. One colon means the option has a required argument. Two
 * colons means the option takes an optional argument.
 */
int x_cliarg_getopt(x_cliarg *cliarg, const x_uchar *optstring);

/**
 * Handles GNU-style long cliarg in addition to getopt() cliarg.
 * This works a lot like GNU's getopt_long(). The last option in
 * longopts must be all zeros, marking the end of the array. The
 * longindex argument may be NULL.
 */
int x_cliarg_getopt_long(x_cliarg *cliarg, const x_cliarg_long *longopts, int *longindex);

/**
 * Used for stepping over non-option arguments.
 * @return the next non-option argument, or NULL for no more arguments
 *
 * Argument parsing can continue with x_cliarg_st() after using this
 * function. That would be used to parse the cliarg for the
 * subcommand returned by x_getopt_arg(). This function allows you to
 * ignore the value of optind.
 */
x_uchar *x_cliarg_arg(x_cliarg *cliarg);

#endif
