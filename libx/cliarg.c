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

#include "x/cliarg.h"
#include "x/string.h"

#define OPT_MSG_INVALID x_u("invalid option")
#define OPT_MSG_MISSING x_u("option requires an argument")
#define OPT_MSG_TOOMANY x_u("option takes no arguments")

static int parser_error(x_cliarg *cliarg, const x_uchar *msg, const x_uchar *data)
{
	unsigned p = 0;
	const x_uchar *sep = x_u(" -- '");
	while (*msg)
		cliarg->errmsg[p++] = *msg++;
	while (*sep)
		cliarg->errmsg[p++] = *sep++;
	while (p < sizeof(cliarg->errmsg) / sizeof(x_uchar) - 2 && *data)
		cliarg->errmsg[p++] = *data++;
	cliarg->errmsg[p++] = '\'';
	cliarg->errmsg[p++] = '\0';
	return x_u('?');
}

void x_cliarg_init(x_cliarg *cliarg, x_uchar **argv)
{
	cliarg->argv = argv;
	cliarg->permute = 1;
	cliarg->optind = argv[0] != 0;
	cliarg->subopt = 0;
	cliarg->optarg = 0;
	cliarg->errmsg[0] = '\0';
}

static int parser_is_dashdash(const x_uchar *arg)
{
	return arg != 0 && arg[0] == '-' && arg[1] == '-' && arg[2] == '\0';
}

static int parser_is_shortopt(const x_uchar *arg)
{
	return arg != 0 && arg[0] == '-' && arg[1] != '-' && arg[1] != '\0';
}

static int parser_is_longopt(const x_uchar *arg)
{
	return arg != 0 && arg[0] == '-' && arg[1] == '-' && arg[2] != '\0';
}

static void parser_permute(x_cliarg *cliarg, int index)
{
	x_uchar *nonoption = cliarg->argv[index];
	int i;
	for (i = index; i < cliarg->optind - 1; i++)
		cliarg->argv[i] = cliarg->argv[i + 1];
	cliarg->argv[cliarg->optind - 1] = nonoption;
}

static int parser_argtype(const x_uchar *optstring, char c)
{
	int count = X_CLIARG_NONE;
	if (c == ':')
		return -1;
	for (; *optstring && c != *optstring; optstring++);
	if (!*optstring)
		return -1;
	if (optstring[1] == x_u(':'))
		count += optstring[2] == x_u(':') ? 2 : 1;
	return count;
}

int x_cliarg_getopt(x_cliarg *cliarg, const x_uchar *optstring)
{
	int type;
	x_uchar *next;
	x_uchar *option = cliarg->argv[cliarg->optind];
	cliarg->errmsg[0] = '\0';
	cliarg->optopt = 0;
	cliarg->optarg = 0;
	if (option == 0) {
		return -1;
	} else if (parser_is_dashdash(option)) {
		cliarg->optind++; /* consume "--" */
		return -1;
	} else if (!parser_is_shortopt(option)) {
		if (cliarg->permute) {
			int index = cliarg->optind++;
			int r = x_cliarg_getopt(cliarg, optstring);
			parser_permute(cliarg, index);
			cliarg->optind--;
			return r;
		} else {
			return -1;
		}
	}
	option += cliarg->subopt + 1;
	cliarg->optopt = option[0];
	type = parser_argtype(optstring, option[0]);
	next = cliarg->argv[cliarg->optind + 1];
	switch (type) {
		case -1:
			;
			x_uchar str[2] = {0, 0};
			str[0] = option[0];
			cliarg->optind++;
			return parser_error(cliarg, OPT_MSG_INVALID, str);
		case X_CLIARG_NONE:
			 if (option[1]) {
				 cliarg->subopt++;
			 } else {
				 cliarg->subopt = 0;
				 cliarg->optind++;
			 }
			 return option[0];
		case X_CLIARG_REQUIRED:
			 cliarg->subopt = 0;
			 cliarg->optind++;
			 if (option[1]) {
				 cliarg->optarg = option + 1;
			 } else if (next != 0) {
				 cliarg->optarg = next;
				 cliarg->optind++;
			 } else {
				 x_uchar str1[2] = {0, 0};
				 str1[0] = option[0];
				 cliarg->optarg = 0;
				 return parser_error(cliarg, OPT_MSG_MISSING, str1);
			 }
			 return option[0];
		case X_CLIARG_OPTIONAL:
			 cliarg->subopt = 0;
			 cliarg->optind++;
			 if (option[1])
				 cliarg->optarg = option + 1;
			 else
				 cliarg->optarg = 0;
			 return option[0];
	}
	return 0;
}

x_uchar *x_cliarg_arg(x_cliarg *cliarg)
{
	x_uchar *option = cliarg->argv[cliarg->optind];
	cliarg->subopt = 0;
	if (option != 0)
		cliarg->optind++;
	return option;
}

static int parser_longopts_end(const x_cliarg_long *longopts, int i)
{
	return !longopts[i].longname && !longopts[i].shortname;
}

static void parser_from_long(const x_cliarg_long *longopts, x_uchar *optstring)
{
	x_uchar *p = optstring;
	int i;
	for (i = 0; !parser_longopts_end(longopts, i); i++) {
		if (longopts[i].shortname && longopts[i].shortname < 127) {
			int a;
			*p++ = longopts[i].shortname;
			for (a = 0; a < (int)longopts[i].argtype; a++)
				*p++ = x_u(':');
		}
	}
	*p = '\0';
}

/* Unlike strcmp(), handles cliarg containing "=". */
static int parser_longopts_match(const x_uchar *longname, const x_uchar *option)
{
	const x_uchar *a = option, *n = longname;
	if (longname == 0)
		return 0;
	for (; *a && *n && *a != x_u('='); a++, n++)
		if (*a != *n)
			return 0;
	return *n == x_u('\0') && (*a == x_u('\0') || *a == x_u('='));
}

/* Return the part after "=", or NULL. */
static x_uchar *parser_longopts_arg(x_uchar *option)
{
	for (; *option && *option != x_u('='); option++);
	if (*option == x_u('='))
		return option + 1;
	else
		return 0;
}

static int parser_long_fallback(x_cliarg *cliarg,
		const x_cliarg_long *longopts, int *longindex)
{
	int result;
	x_uchar optstring[96 * 3 + 1]; /* 96 ASCII printable characters */
	parser_from_long(longopts, optstring);
	result = x_cliarg_getopt(cliarg, optstring);
	if (longindex != 0) {
		*longindex = -1;
		if (result != -1) {
			int i;
			for (i = 0; !parser_longopts_end(longopts, i); i++)
				if (longopts[i].shortname == cliarg->optopt)
					*longindex = i;
		}
	}
	return result;
}

int x_cliarg_getopt_long(x_cliarg *cliarg,
		const x_cliarg_long *longopts, int *longindex)
{
	int i;
	x_uchar *option = cliarg->argv[cliarg->optind];
	if (option == 0) {
		return -1;
	} else if (parser_is_dashdash(option)) {
		cliarg->optind++; /* consume "--" */
		return -1;
	} else if (parser_is_shortopt(option)) {
		return parser_long_fallback(cliarg, longopts, longindex);
	} else if (!parser_is_longopt(option)) {
		if (cliarg->permute) {
			int index = cliarg->optind++;
			int r = x_cliarg_getopt_long(cliarg, longopts, longindex);
			parser_permute(cliarg, index);
			cliarg->optind--;
			return r;
		} else {
			return -1;
		}
	}
	/* Parse as long option. */
	cliarg->errmsg[0] = '\0';
	cliarg->optopt = 0;
	cliarg->optarg = 0;
	option += 2; /* skip "--" */
	cliarg->optind++;
	for (i = 0; !parser_longopts_end(longopts, i); i++) {
		const x_uchar *name = longopts[i].longname;
		if (parser_longopts_match(name, option)) {
			x_uchar *arg;
			if (longindex)
				*longindex = i;
			cliarg->optopt = longopts[i].shortname;
			arg = parser_longopts_arg(option);
			if (longopts[i].argtype == X_CLIARG_NONE && arg != 0) {
				return parser_error(cliarg, OPT_MSG_TOOMANY, name);
			} if (arg != 0) {
				cliarg->optarg = arg;
			} else if (longopts[i].argtype == X_CLIARG_REQUIRED) {
				cliarg->optarg = cliarg->argv[cliarg->optind];
				if (cliarg->optarg == 0)
					return parser_error(cliarg, OPT_MSG_MISSING, name);
				else
					cliarg->optind++;
			}
			return cliarg->optopt;
		}
	}
	return parser_error(cliarg, OPT_MSG_INVALID, option);
}

