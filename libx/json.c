/*
  Copyright (c) 2009-2017 Dave Gamble and cJSON contributors
  Copyright (c) 2025 Li Xilin <lixilin@gmx.com>

  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in
  all copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
  THE SOFTWARE.
*/

/* disable warnings about old C89 functions in MSVC */
#if !defined(_CRT_SECURE_NO_DEPRECATE) && defined(_MSC_VER)
#define _CRT_SECURE_NO_DEPRECATE
#endif

#ifdef __GNUC__
#pragma GCC visibility push(default)
#endif
#if defined(_MSC_VER)
#pragma warning (push)
/* disable warning about single line comments in system headers */
#pragma warning (disable : 4001)
#endif

#include "x/json.h"
#include "x/macros.h"
#include "x/memory.h"
#include <string.h>
#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <limits.h>
#include <ctype.h>
#include <float.h>
#include <stdbool.h>
#include <assert.h>

#ifdef ENABLE_LOCALES
#include <locale.h>
#endif

#if 0
#if defined(_MSC_VER)
#pragma warning (pop)
#endif
#ifdef __GNUC__
#pragma GCC visibility pop
#endif
#endif

/* define isnan and isinf for ANSI C, if in C99 or above, isnan and isinf has been defined in math.h */
#ifndef isinf
#define isinf(d) (isnan((d - d)) && !isnan(d))
#endif
#ifndef isnan
#define isnan(d) (d != d)
#endif

#ifndef NAN
#ifdef _WIN32
#define NAN sqrt(-1.0)
#else
#define NAN 0.0/0.0
#endif
#endif

typedef struct {
    const uint8_t *json;
    size_t position;
} error;

static error global_error = { NULL, 0 };

const char * x_json_get_error_ptr(void)
{
    return (const char*) (global_error.json + global_error.position);
}

char * x_json_get_string_value(const x_json *const item)
{
    if (!x_json_is_string(item)) {
        return NULL;
    }

    return item->valuestring;
}

double x_json_get_number_value(const x_json *const item)
{
    if (!x_json_is_number(item))
        return (double) NAN;
    return item->valuedouble;
}

/* Case insensitive string comparison, doesn't consider two NULL pointers equal though */
static int case_insensitive_strcmp(const uint8_t *string1, const uint8_t *string2)
{
    if ((string1 == NULL) || (string2 == NULL))
        return 1;
    if (string1 == string2)
        return 0;
    for(; tolower(*string1) == tolower(*string2); (void)string1++, string2++) {
        if (*string1 == '\0')
            return 0;
    }
    return tolower(*string1) - tolower(*string2);
}

#define static_strlen(string_literal) (sizeof(string_literal) - sizeof(""))

static uint8_t* x_json_strdup(const uint8_t* string)
{
    size_t length = 0;
    uint8_t *copy = NULL;
	assert(string);
    length = strlen((const char*)string) + sizeof("");
    copy = (uint8_t*)x_malloc(NULL, length);
    memcpy(copy, string, length);
    return copy;
}

/* Internal constructor. */
static x_json *x_json_new_item(void)
{
    x_json* node = (x_json*)x_malloc(NULL, sizeof(x_json));
    if (node) {
        memset(node, '\0', sizeof(x_json));
    }

    return node;
}

/* Delete a x_json structure. */
void x_json_delete(x_json *item)
{
    x_json *next = NULL;
    while (item != NULL) {
        next = item->next;
        if (!(item->type & X_JSON_IS_REF) && (item->child != NULL)) {
            x_json_delete(item->child);
        }
        if (!(item->type & X_JSON_IS_REF) && (item->valuestring != NULL)) {
            x_free(item->valuestring);
            item->valuestring = NULL;
        }
        if (!(item->type & X_JSON_STRING_IS_CONST) && (item->string != NULL)) {
            x_free(item->string);
            item->string = NULL;
        }
        x_free(item);
        item = next;
    }
}

/* get the decimal point character of the current locale */
static uint8_t get_decimal_point(void)
{
#ifdef ENABLE_LOCALES
    struct lconv *lconv = localeconv();
    return (uint8_t) lconv->decimal_point[0];
#else
    return '.';
#endif
}

typedef struct
{
    const uint8_t *content;
    size_t length;
    size_t offset;
    size_t depth; /* How deeply nested (in arrays/objects) is the input at the current offset. */
} parse_buffer;

/* check if the given size is left to read in a given parse buffer (starting with 1) */
#define can_read(buffer, size) ((buffer != NULL) && (((buffer)->offset + size) <= (buffer)->length))
/* check if the buffer can be accessed at the given index (starting with 0) */
#define can_access_at_index(buffer, index) ((buffer != NULL) && (((buffer)->offset + index) < (buffer)->length))
#define cannot_access_at_index(buffer, index) (!can_access_at_index(buffer, index))
/* get a pointer to the buffer at the position */
#define buffer_at_offset(buffer) ((buffer)->content + (buffer)->offset)

/* Parse the input text to generate a number, and populate the result into item. */
static bool parse_number(x_json *const item, parse_buffer * const input_buffer)
{
    double number = 0;
    uint8_t *after_end = NULL;
    uint8_t *number_c_string;
    uint8_t decimal_point = get_decimal_point();
    size_t i = 0;
    size_t number_string_length = 0;
    bool has_decimal_point = false;
    if ((input_buffer == NULL) || (input_buffer->content == NULL))
        return false;
    /* copy the number into a temporary buffer and replace '.' with the decimal point
     * of the current locale (for strtod)
     * This also takes care of '\0' not necessarily being available for marking the end of the input */
    for (i = 0; can_access_at_index(input_buffer, i); i++) {
        switch (buffer_at_offset(input_buffer)[i]) {
            case '0': case '1': case '2': case '3': case '4': case '5': case '6': case '7':
            case '8': case '9': case '+': case '-': case 'e': case 'E':
                number_string_length++;
                break;
            case '.':
                number_string_length++;
                has_decimal_point = true;
                break;
            default:
                goto loop_end;
        }
    }
loop_end:
    number_c_string = (uint8_t *) x_malloc(NULL, number_string_length + 1);
    if (number_c_string == NULL)
        return false;
    memcpy(number_c_string, buffer_at_offset(input_buffer), number_string_length);
    number_c_string[number_string_length] = '\0';
    if (has_decimal_point) {
        for (i = 0; i < number_string_length; i++) {
            if (number_c_string[i] == '.') {
                /* replace '.' with the decimal point of the current locale (for strtod) */
                number_c_string[i] = decimal_point;
            }
        }
    }
    number = strtod((const char*)number_c_string, (char**)&after_end);
    if (number_c_string == after_end) {
        /* free the temporary buffer */
        x_free(number_c_string);
        return false; /* parse_error */
    }
    item->valuedouble = number;
    /* use saturation in case of overflow */
    if (number >= INT_MAX)
        item->valueint = INT_MAX;
    else if (number <= (double)INT_MIN)
        item->valueint = INT_MIN;
    else
        item->valueint = (int)number;
    item->type = X_JSON_NUMBER;
    input_buffer->offset += (size_t)(after_end - number_c_string);
    x_free(number_c_string);
    return true;
}

/* don't ask me, but the original x_json_set_number_value returns an integer or double */
double x_json_set_number_helper(x_json *object, double number)
{
    if (number >= INT_MAX)
        object->valueint = INT_MAX;
    else if (number <= (double)INT_MIN)
        object->valueint = INT_MIN;
    else
        object->valueint = (int)number;
    return object->valuedouble = number;
}

/* Note: when passing a NULL valuestring, x_json_set_valuestring treats this as an error and return NULL */
char* x_json_set_valuestring(x_json *object, const char *valuestring)
{
    char *copy = NULL;
    size_t v1_len;
    size_t v2_len;
    /* if object's type is not X_JSON_STRING or is X_JSON_IS_REF, it should not set valuestring */
    if ((object == NULL) || !(object->type & X_JSON_STRING) || (object->type & X_JSON_IS_REF))
        return NULL;
    /* return NULL if the object is corrupted or valuestring is NULL */
    if (object->valuestring == NULL || valuestring == NULL)
        return NULL;
    v1_len = strlen(valuestring);
    v2_len = strlen(object->valuestring);
    if (v1_len <= v2_len) {
        /* strcpy does not handle overlapping string: [X1, X2] [Y1, Y2] => X2 < Y1 or Y2 < X1 */
        if (!( valuestring + v1_len < object->valuestring || object->valuestring + v2_len < valuestring )) {
            return NULL;
        }
        strcpy(object->valuestring, valuestring);
        return object->valuestring;
    }
    copy = (char*) x_json_strdup((const uint8_t*)valuestring);
    if (object->valuestring != NULL)
        x_json_free(object->valuestring);
    object->valuestring = copy;
    return copy;
}

typedef struct
{
    uint8_t *buffer;
    size_t length;
    size_t offset;
    size_t depth; /* current nesting depth (for formatted printing) */
    bool noalloc;
    bool format; /* is this print a formatted print */
} printbuffer;

/* realloc printbuffer if necessary to have at least "needed" bytes more */
static uint8_t* ensure(printbuffer * const p, size_t needed)
{
    uint8_t *newbuffer = NULL;
    size_t newsize = 0;
    if ((p == NULL) || (p->buffer == NULL))
        return NULL;
    if ((p->length > 0) && (p->offset >= p->length))
        /* make sure that offset is valid */
        return NULL;
    if (needed > INT_MAX)
        /* sizes bigger than INT_MAX are currently not supported */
        return NULL;
    needed += p->offset + 1;
    if (needed <= p->length)
        return p->buffer + p->offset;
    if (p->noalloc)
        return NULL;
    /* calculate new buffer size */
    if (needed > (INT_MAX / 2)) {
        /* overflow of int, use INT_MAX if possible */
        if (needed <= INT_MAX)
            newsize = INT_MAX;
        else
            return NULL;
    }
    else {
        newsize = needed * 2;
	}
	/* reallocate with realloc if available */
	newbuffer = (uint8_t*)x_realloc(p->buffer, newsize);
	p->length = newsize;
    p->buffer = newbuffer;
    return newbuffer + p->offset;
}

/* calculate the new length of the string in a printbuffer and update the offset */
static void update_offset(printbuffer * const buffer)
{
    const uint8_t *buffer_pointer = NULL;
    if ((buffer == NULL) || (buffer->buffer == NULL))
        return;
    buffer_pointer = buffer->buffer + buffer->offset;
    buffer->offset += strlen((const char*)buffer_pointer);
}

/* securely comparison of floating-point variables */
static bool compare_double(double a, double b)
{
    double maxVal = fabs(a) > fabs(b) ? fabs(a) : fabs(b);
    return (fabs(a - b) <= maxVal * DBL_EPSILON);
}

/* Render the number nicely from the given item into a string. */
static bool print_number(const x_json *const item, printbuffer * const output_buffer)
{
    uint8_t *output_pointer = NULL;
    double d = item->valuedouble;
    int length = 0;
    size_t i = 0;
    uint8_t number_buffer[26] = {0}; /* temporary buffer to print the number into */
    uint8_t decimal_point = get_decimal_point();
    double test = 0.0;

    if (output_buffer == NULL)
        return false;
    /* This checks for NaN and Infinity */
    if (isnan(d) || isinf(d))
        length = sprintf((char*)number_buffer, "null");
    else if(d == (double)item->valueint)
        length = sprintf((char*)number_buffer, "%d", item->valueint);
    else {
        /* Try 15 decimal places of precision to avoid nonsignificant nonzero digits */
        length = sprintf((char*)number_buffer, "%1.15g", d);

        /* Check whether the original double can be recovered */
        if ((sscanf((char*)number_buffer, "%lg", &test) != 1) || !compare_double((double)test, d)) {
            /* If not, print with 17 decimal places of precision */
            length = sprintf((char*)number_buffer, "%1.17g", d);
        }
    }
    /* sprintf failed or buffer overrun occurred */
    if ((length < 0) || (length > (int)(sizeof(number_buffer) - 1)))
        return false;
    /* reserve appropriate space in the output */
    output_pointer = ensure(output_buffer, (size_t)length + sizeof(""));
    if (!output_pointer)
        return false;
    /* copy the printed number to the output and replace locale
     * dependent decimal point with '.' */
    for (i = 0; i < ((size_t)length); i++) {
        if (number_buffer[i] == decimal_point) {
            output_pointer[i] = '.';
            continue;
        }

        output_pointer[i] = number_buffer[i];
    }
    output_pointer[i] = '\0';
    output_buffer->offset += (size_t)length;
    return true;
}

/* parse 4 digit hexadecimal number */
static uint32_t parse_hex4(const uint8_t * const input)
{
    uint32_t h = 0;
    size_t i = 0;
    for (i = 0; i < 4; i++) {
        /* parse digit */
        if ((input[i] >= '0') && (input[i] <= '9'))
            h += (uint32_t) input[i] - '0';
        else if ((input[i] >= 'A') && (input[i] <= 'F'))
            h += (uint32_t) 10 + input[i] - 'A';
        else if ((input[i] >= 'a') && (input[i] <= 'f'))
            h += (uint32_t) 10 + input[i] - 'a';
        else
            return 0;
        if (i < 3)
            /* shift left to make place for the next nibble */
            h = h << 4;
    }

    return h;
}

/* converts a UTF-16 literal to UTF-8
 * A literal can be one or two sequences of the form \uXXXX */
static uint8_t utf16_literal_to_utf8(const uint8_t * const input_pointer, const uint8_t * const input_end, uint8_t **output_pointer)
{
    uint32_t codepoint = 0;
    unsigned int first_code = 0;
    const uint8_t *first_sequence = input_pointer;
    uint8_t utf8_length = 0, utf8_position = 0, sequence_length = 0, first_byte_mark = 0;
    if ((input_end - first_sequence) < 6)
        /* input ends unexpectedly */
        goto fail;
    /* get the first utf16 sequence */
    first_code = parse_hex4(first_sequence + 2);
    /* check that the code is valid */
    if (((first_code >= 0xDC00) && (first_code <= 0xDFFF)))
        goto fail;
    /* UTF16 surrogate pair */
    if ((first_code >= 0xD800) && (first_code <= 0xDBFF)) {
        const uint8_t *second_sequence = first_sequence + 6;
        unsigned int second_code = 0;
        sequence_length = 12; /* \uXXXX\uXXXX */

        if ((input_end - second_sequence) < 6)
            /* input ends unexpectedly */
            goto fail;
        if ((second_sequence[0] != '\\') || (second_sequence[1] != 'u'))
            /* missing second half of the surrogate pair */
            goto fail;
        /* get the second utf16 sequence */
        second_code = parse_hex4(second_sequence + 2);
        /* check that the code is valid */
        if ((second_code < 0xDC00) || (second_code > 0xDFFF))
            /* invalid second half of the surrogate pair */
            goto fail;
        /* calculate the unicode codepoint from the surrogate pair */
        codepoint = 0x10000 + (((first_code & 0x3FF) << 10) | (second_code & 0x3FF));
    }
    else {
        sequence_length = 6; /* \uXXXX */
        codepoint = first_code;
    }
    /* encode as UTF-8
     * takes at maximum 4 bytes to encode:
     * 11110xxx 10xxxxxx 10xxxxxx 10xxxxxx */
    if (codepoint < 0x80)
        /* normal ascii, encoding 0xxxxxxx */
        utf8_length = 1;
    else if (codepoint < 0x800) {
        /* two bytes, encoding 110xxxxx 10xxxxxx */
        utf8_length = 2;
        first_byte_mark = 0xC0; /* 11000000 */
    }
    else if (codepoint < 0x10000) {
        /* three bytes, encoding 1110xxxx 10xxxxxx 10xxxxxx */
        utf8_length = 3;
        first_byte_mark = 0xE0; /* 11100000 */
    }
    else if (codepoint <= 0x10FFFF) {
        /* four bytes, encoding 1110xxxx 10xxxxxx 10xxxxxx 10xxxxxx */
        utf8_length = 4;
        first_byte_mark = 0xF0; /* 11110000 */
    }
    else
        /* invalid unicode codepoint */
        goto fail;
    /* encode as utf8 */
    for (utf8_position = (uint8_t)(utf8_length - 1); utf8_position > 0; utf8_position--) {
        /* 10xxxxxx */
        (*output_pointer)[utf8_position] = (uint8_t)((codepoint | 0x80) & 0xBF);
        codepoint >>= 6;
    }
    /* encode first byte */
    if (utf8_length > 1)
        (*output_pointer)[0] = (uint8_t)((codepoint | first_byte_mark) & 0xFF);
    else
        (*output_pointer)[0] = (uint8_t)(codepoint & 0x7F);
    *output_pointer += utf8_length;
    return sequence_length;
fail:
    return 0;
}

/* Parse the input text into an unescaped cinput, and populate item. */
static bool parse_string(x_json *const item, parse_buffer * const input_buffer)
{
    const uint8_t *input_pointer = buffer_at_offset(input_buffer) + 1;
    const uint8_t *input_end = buffer_at_offset(input_buffer) + 1;
    uint8_t *output_pointer = NULL, *output = NULL;
	size_t allocation_length = 0;
	size_t skipped_bytes = 0;
    /* not a string */
    if (buffer_at_offset(input_buffer)[0] != '\"') {
        goto fail;
	}

	/* calculate approximate size of the output (overestimate) */
	while (((size_t)(input_end - input_buffer->content) < input_buffer->length) && (*input_end != '\"')) {
		/* is escape sequence */
		if (input_end[0] == '\\') {
			if ((size_t)(input_end + 1 - input_buffer->content) >= input_buffer->length) {
				/* prevent buffer overflow when last input character is a backslash */
				goto fail;
			}
			skipped_bytes++;
			input_end++;
		}
		input_end++;
	}
	if (((size_t)(input_end - input_buffer->content) >= input_buffer->length) || (*input_end != '\"')) {
		goto fail; /* string ended unexpectedly */
	}

	/* This is at most how much we need for the output */
	allocation_length = (size_t) (input_end - buffer_at_offset(input_buffer)) - skipped_bytes;
	output = (uint8_t*)x_malloc(NULL, allocation_length + sizeof(""));

    output_pointer = output;
    /* loop through the string literal */
    while (input_pointer < input_end) {
        if (*input_pointer != '\\') {
            *output_pointer++ = *input_pointer++;
			continue;
		}
		uint8_t sequence_length = 2;
		if ((input_end - input_pointer) < 1)
			goto fail;
		switch (input_pointer[1]) {
			case 'b':
				*output_pointer++ = '\b';
				break;
			case 'f':
				*output_pointer++ = '\f';
				break;
			case 'n':
				*output_pointer++ = '\n';
				break;
			case 'r':
				*output_pointer++ = '\r';
				break;
			case 't':
				*output_pointer++ = '\t';
				break;
			case '\"':
			case '\\':
			case '/':
				*output_pointer++ = input_pointer[1];
				break;

				/* UTF-16 literal */
			case 'u':
				sequence_length = utf16_literal_to_utf8(input_pointer, input_end, &output_pointer);
				if (sequence_length == 0) {
					/* failed to convert UTF16-literal to UTF-8 */
					goto fail;
				}
				break;

			default:
				goto fail;
		}
		input_pointer += sequence_length;
	}

    /* zero terminate the output */
    *output_pointer = '\0';
    item->type = X_JSON_STRING;
    item->valuestring = (char*)output;
    input_buffer->offset = (size_t) (input_end - input_buffer->content);
    input_buffer->offset++;
    return true;

fail:
    if (output != NULL) {
        x_free(output);
        output = NULL;
    }
    if (input_pointer != NULL)
        input_buffer->offset = (size_t)(input_pointer - input_buffer->content);
    return false;
}

/* Render the cstring provided to an escaped version that can be printed. */
static bool print_string_ptr(const uint8_t * const input, printbuffer * const output_buffer)
{
    const uint8_t *input_pointer = NULL;
    uint8_t *output = NULL;
    uint8_t *output_pointer = NULL;
    size_t output_length = 0;
    /* numbers of additional characters needed for escaping */
    size_t escape_characters = 0;
    if (output_buffer == NULL)
        return false;
    /* empty string */
    if (input == NULL) {
        output = ensure(output_buffer, sizeof("\"\""));
        if (!output)
            return false;
        strcpy((char*)output, "\"\"");
        return true;
    }
    /* set "flag" to 1 if something needs to be escaped */
    for (input_pointer = input; *input_pointer; input_pointer++) {
        switch (*input_pointer) {
            case '\"':
            case '\\':
            case '\b':
            case '\f':
            case '\n':
            case '\r':
            case '\t':
                /* one character escape sequence */
                escape_characters++;
                break;
            default:
                if (*input_pointer < 32) {
                    /* UTF-16 escape sequence uXXXX */
                    escape_characters += 5;
                }
                break;
        }
    }
    output_length = (size_t)(input_pointer - input) + escape_characters;
    output = ensure(output_buffer, output_length + sizeof("\"\""));
    if (!output)
        return false;
    /* no characters have to be escaped */
    if (escape_characters == 0) {
        output[0] = '\"';
        memcpy(output + 1, input, output_length);
        output[output_length + 1] = '\"';
        output[output_length + 2] = '\0';
        return true;
    }
    output[0] = '\"';
    output_pointer = output + 1;
    /* copy the string */
    for (input_pointer = input; *input_pointer != '\0'; (void)input_pointer++, output_pointer++) {
        if ((*input_pointer > 31) && (*input_pointer != '\"') && (*input_pointer != '\\')) {
            /* normal character, copy */
            *output_pointer = *input_pointer;
			continue;
        }
		/* character needs to be escaped */
		*output_pointer++ = '\\';
		switch (*input_pointer) {
			case '\\':
				*output_pointer = '\\';
				break;
			case '\"':
				*output_pointer = '\"';
				break;
			case '\b':
				*output_pointer = 'b';
				break;
			case '\f':
				*output_pointer = 'f';
				break;
			case '\n':
				*output_pointer = 'n';
				break;
			case '\r':
				*output_pointer = 'r';
				break;
			case '\t':
				*output_pointer = 't';
				break;
			default:
				/* escape and print as unicode codepoint */
				sprintf((char*)output_pointer, "u%04x", *input_pointer);
				output_pointer += 4;
				break;
		}
    }
    output[output_length + 1] = '\"';
    output[output_length + 2] = '\0';
    return true;
}

/* Invoke print_string_ptr (which is useful) on an item. */
static bool print_string(const x_json *const item, printbuffer * const p)
{
    return print_string_ptr((uint8_t*)item->valuestring, p);
}

/* Predeclare these prototypes. */
static bool parse_value(x_json *const item, parse_buffer * const input_buffer);
static bool print_value(const x_json *const item, printbuffer * const output_buffer);
static bool parse_array(x_json *const item, parse_buffer * const input_buffer);
static bool print_array(const x_json *const item, printbuffer * const output_buffer);
static bool parse_object(x_json *const item, parse_buffer * const input_buffer);
static bool print_object(const x_json *const item, printbuffer * const output_buffer);

/* Utility to jump whitespace and cr/lf */
static parse_buffer *buffer_skip_whitespace(parse_buffer * const buffer)
{
    if ((buffer == NULL) || (buffer->content == NULL)) {
        return NULL;
    }

    if (cannot_access_at_index(buffer, 0)) {
        return buffer;
    }

    while (can_access_at_index(buffer, 0) && (buffer_at_offset(buffer)[0] <= 32)) {
       buffer->offset++;
    }

    if (buffer->offset == buffer->length) {
        buffer->offset--;
    }

    return buffer;
}

/* skip the UTF-8 BOM (byte order mark) if it is at the beginning of a buffer */
static parse_buffer *skip_utf8_bom(parse_buffer * const buffer)
{
    if ((buffer == NULL) || (buffer->content == NULL) || (buffer->offset != 0)) {
        return NULL;
    }

    if (can_access_at_index(buffer, 4) && (strncmp((const char*)buffer_at_offset(buffer), "\xEF\xBB\xBF", 3) == 0)) {
        buffer->offset += 3;
    }

    return buffer;
}

x_json *x_json_parse3(const char *value, const char **return_parse_end, bool require_null_terminated)
{
    size_t buffer_length;

    if (NULL == value) {
        return NULL;
    }

    /* Adding null character size due to require_null_terminated. */
    buffer_length = strlen(value) + sizeof("");

    return x_json_parse4(value, buffer_length, return_parse_end, require_null_terminated);
}

/* Parse an object - create a new root, and populate. */
x_json *x_json_parse4(const char *value, size_t buffer_length, const char **return_parse_end, bool require_null_terminated)
{
    parse_buffer buffer = { 0 };
    x_json *item = NULL;

    /* reset error position */
    global_error.json = NULL;
    global_error.position = 0;

    if (value == NULL || 0 == buffer_length)
        goto fail;

    buffer.content = (const uint8_t*)value;
    buffer.length = buffer_length;
    buffer.offset = 0;

    item = x_json_new_item();
    if (item == NULL)
        goto fail;

    if (!parse_value(item, buffer_skip_whitespace(skip_utf8_bom(&buffer)))) {
        /* parse failure. ep is set. */
        goto fail;
    }

    /* if we require null-terminated JSON without appended garbage, skip and then check for a null terminator */
    if (require_null_terminated) {
        buffer_skip_whitespace(&buffer);
        if ((buffer.offset >= buffer.length) || buffer_at_offset(&buffer)[0] != '\0') {
            goto fail;
        }
    }
    if (return_parse_end) {
        *return_parse_end = (const char*)buffer_at_offset(&buffer);
    }
    return item;
fail:
    if (item != NULL)
        x_json_delete(item);
    if (value != NULL) {
        error local_error;
        local_error.json = (const uint8_t*)value;
        local_error.position = 0;

        if (buffer.offset < buffer.length)
            local_error.position = buffer.offset;
        else if (buffer.length > 0)
            local_error.position = buffer.length - 1;
        if (return_parse_end)
            *return_parse_end = (const char*)local_error.json + local_error.position;
        global_error = local_error;
    }
    return NULL;
}

x_json *x_json_parse(const char *value)
{
    return x_json_parse3(value, 0, 0);
}

x_json *x_json_parse2(const char *value, size_t buffer_length)
{
    return x_json_parse4(value, buffer_length, 0, 0);
}

static uint8_t *print(const x_json *const item, bool format)
{
    static const size_t default_buffer_size = 256;
    printbuffer buffer[1];
    uint8_t *printed = NULL;

    memset(buffer, 0, sizeof(buffer));

    /* create buffer */
    buffer->buffer = (uint8_t*) x_malloc(NULL, default_buffer_size);
    buffer->length = default_buffer_size;
    buffer->format = format;
    if (buffer->buffer == NULL)
        goto fail;

    /* print the value */
    if (!print_value(item, buffer))
        goto fail;
    update_offset(buffer);

	printed = (uint8_t*) x_realloc(buffer->buffer, buffer->offset + 1);
	if (printed == NULL) {
		goto fail;
	}
	buffer->buffer = NULL;
	
    return printed;
fail:
    if (buffer->buffer != NULL) {
        x_free(buffer->buffer);
        buffer->buffer = NULL;
    }
    if (printed != NULL) {
        x_free(printed);
        printed = NULL;
    }
    return NULL;
}

/* Render a x_json item/entity/structure to text. */
char * x_json_print(const x_json *item, bool fmt)
{
    return (char*)print(item, fmt);
}

char * x_json_print_buffered(const x_json *item, int prebuffer, bool fmt)
{
    printbuffer p = { 0 };
    if (prebuffer < 0)
        return NULL;
    p.buffer = (uint8_t*)x_malloc(NULL, (size_t)prebuffer);
    p.length = (size_t)prebuffer;
    p.offset = 0;
    p.noalloc = false;
    p.format = fmt;
    if (!print_value(item, &p)) {
        x_free(p.buffer);
        p.buffer = NULL;
        return NULL;
    }
    return (char*)p.buffer;
}

bool x_json_print_to_buffer(x_json *item, char *buffer, const int length, const bool format)
{
    printbuffer p = { 0 };
    if ((length < 0) || (buffer == NULL))
        return false;
    p.buffer = (uint8_t*)buffer;
    p.length = (size_t)length;
    p.offset = 0;
    p.noalloc = true;
    p.format = format;
    return print_value(item, &p);
}

/* Parser core - when encountering text, process appropriately. */
static bool parse_value(x_json *const item, parse_buffer * const input_buffer)
{
    if ((input_buffer == NULL) || (input_buffer->content == NULL))
        return false; /* no input */
    /* parse the different types of values */
    /* null */
    if (can_read(input_buffer, 4) && (strncmp((const char*)buffer_at_offset(input_buffer), "null", 4) == 0)) {
        item->type = x_json_NULL;
        input_buffer->offset += 4;
        return true;
    }
    /* false */
    if (can_read(input_buffer, 5) && (strncmp((const char*)buffer_at_offset(input_buffer), "false", 5) == 0)) {
        item->type = X_JSON_FALSE;
        input_buffer->offset += 5;
        return true;
    }
    /* true */
    if (can_read(input_buffer, 4) && (strncmp((const char*)buffer_at_offset(input_buffer), "true", 4) == 0)) {
        item->type = X_JSON_TRUE;
        item->valueint = 1;
        input_buffer->offset += 4;
        return true;
    }
    /* string */
    if (can_access_at_index(input_buffer, 0) && (buffer_at_offset(input_buffer)[0] == '\"')) {
        return parse_string(item, input_buffer);
    }
    /* number */
    if (can_access_at_index(input_buffer, 0) && ((buffer_at_offset(input_buffer)[0] == '-') || ((buffer_at_offset(input_buffer)[0] >= '0') && (buffer_at_offset(input_buffer)[0] <= '9')))) {
        return parse_number(item, input_buffer);
    }
    /* array */
    if (can_access_at_index(input_buffer, 0) && (buffer_at_offset(input_buffer)[0] == '[')) {
        return parse_array(item, input_buffer);
    }
    /* object */
    if (can_access_at_index(input_buffer, 0) && (buffer_at_offset(input_buffer)[0] == '{')) {
        return parse_object(item, input_buffer);
    }
    return false;
}

/* Render a value to text. */
static bool print_value(const x_json *const item, printbuffer * const output_buffer)
{
    uint8_t *output = NULL;
    if ((item == NULL) || (output_buffer == NULL))
        return false;
    switch ((item->type) & 0xFF) {
        case x_json_NULL:
            output = ensure(output_buffer, 5);
            if (!output)
                return false;
            strcpy((char*)output, "null");
            return true;

        case X_JSON_FALSE:
            output = ensure(output_buffer, 6);
            if (!output)
                return false;
            strcpy((char*)output, "false");
            return true;

        case X_JSON_TRUE:
            output = ensure(output_buffer, 5);
            if (!output)
                return false;
            strcpy((char*)output, "true");
            return true;
        case X_JSON_NUMBER:
            return print_number(item, output_buffer);
        case X_JSON_RAW:
            if (item->valuestring == NULL)
                return false;
            size_t raw_length = 0;
            raw_length = strlen(item->valuestring) + sizeof("");
            output = ensure(output_buffer, raw_length);
            if (!output)
                return false;
            memcpy(output, item->valuestring, raw_length);
            return true;
        case X_JSON_STRING:
            return print_string(item, output_buffer);
        case X_JSON_ARRAY:
            return print_array(item, output_buffer);
        case X_JSON_OBJECT:
            return print_object(item, output_buffer);
        default:
            return false;
    }
}

/* Build an array from input text. */
static bool parse_array(x_json *const item, parse_buffer * const input_buffer)
{
    x_json *head = NULL; /* head of the linked list */
    x_json *current_item = NULL;
    if (input_buffer->depth >= X_JSON_NESTING_LIMIT)
        return false; /* to deeply nested */
    input_buffer->depth++;
    if (buffer_at_offset(input_buffer)[0] != '[')
        /* not an array */
        goto fail;
    input_buffer->offset++;
    buffer_skip_whitespace(input_buffer);
    if (can_access_at_index(input_buffer, 0) && (buffer_at_offset(input_buffer)[0] == ']'))
        /* empty array */
        goto success;
    /* check if we skipped to the end of the buffer */
    if (cannot_access_at_index(input_buffer, 0)) {
        input_buffer->offset--;
        goto fail;
    }
    /* step back to character in front of the first element */
    input_buffer->offset--;
    /* loop through the comma separated array elements */
    do {
        /* allocate next item */
        x_json *new_item = x_json_new_item();
        if (new_item == NULL)
            goto fail; /* allocation failure */
        /* attach next item to list */
        if (head == NULL)
            /* start the linked list */
            current_item = head = new_item;
        else {
            /* add to the end and advance */
            current_item->next = new_item;
            new_item->prev = current_item;
            current_item = new_item;
        }
        /* parse next value */
        input_buffer->offset++;
        buffer_skip_whitespace(input_buffer);
        if (!parse_value(current_item, input_buffer))
            goto fail; /* failed to parse value */
        buffer_skip_whitespace(input_buffer);
    }
    while (can_access_at_index(input_buffer, 0) && (buffer_at_offset(input_buffer)[0] == ','));
    if (cannot_access_at_index(input_buffer, 0) || buffer_at_offset(input_buffer)[0] != ']')
        goto fail; /* expected end of array */
success:
    input_buffer->depth--;
    if (head != NULL)
        head->prev = current_item;
    item->type = X_JSON_ARRAY;
    item->child = head;
    input_buffer->offset++;
    return true;
fail:
    if (head != NULL)
        x_json_delete(head);
    return false;
}

/* Render an array to text */
static bool print_array(const x_json *const item, printbuffer * const output_buffer)
{
    uint8_t *output_pointer = NULL;
    size_t length = 0;
    x_json *current_element = item->child;
    if (output_buffer == NULL)
        return false;
    /* Compose the output array. */
    /* opening square bracket */
    output_pointer = ensure(output_buffer, 1);
    if (!output_pointer)
        return false;
    *output_pointer = '[';
    output_buffer->offset++;
    output_buffer->depth++;

    while (current_element != NULL) {
        if (!print_value(current_element, output_buffer))
            return false;
        update_offset(output_buffer);
        if (current_element->next) {
            length = (size_t) (output_buffer->format ? 2 : 1);
            output_pointer = ensure(output_buffer, length + 1);
            if (!output_pointer)
                return false;
            *output_pointer++ = ',';
            if(output_buffer->format)
                *output_pointer++ = ' ';
            *output_pointer = '\0';
            output_buffer->offset += length;
        }
        current_element = current_element->next;
    }
    output_pointer = ensure(output_buffer, 2);
    if (!output_pointer)
        return false;
    *output_pointer++ = ']';
    *output_pointer = '\0';
    output_buffer->depth--;
    return true;
}

/* Build an object from the text. */
static bool parse_object(x_json *const item, parse_buffer * const input_buffer)
{
    x_json *head = NULL; /* linked list head */
    x_json *current_item = NULL;
    if (input_buffer->depth >= X_JSON_NESTING_LIMIT)
        return false; /* to deeply nested */
    input_buffer->depth++;
    if (cannot_access_at_index(input_buffer, 0) || (buffer_at_offset(input_buffer)[0] != '{'))
        goto fail; /* not an object */
    input_buffer->offset++;
    buffer_skip_whitespace(input_buffer);
    if (can_access_at_index(input_buffer, 0) && (buffer_at_offset(input_buffer)[0] == '}'))
        goto success; /* empty object */
    /* check if we skipped to the end of the buffer */
    if (cannot_access_at_index(input_buffer, 0)) {
        input_buffer->offset--;
        goto fail;
    }
    /* step back to character in front of the first element */
    input_buffer->offset--;
    /* loop through the comma separated array elements */
    do {
        /* allocate next item */
        x_json *new_item = x_json_new_item();
        if (new_item == NULL)
            goto fail; /* allocation failure */
        /* attach next item to list */
        if (head == NULL)
            /* start the linked list */
            current_item = head = new_item;
        else {
            /* add to the end and advance */
            current_item->next = new_item;
            new_item->prev = current_item;
            current_item = new_item;
        }
        if (cannot_access_at_index(input_buffer, 1))
            goto fail; /* nothing comes after the comma */
        /* parse the name of the child */
        input_buffer->offset++;
        buffer_skip_whitespace(input_buffer);
        if (!parse_string(current_item, input_buffer))
            goto fail; /* failed to parse name */
        buffer_skip_whitespace(input_buffer);
        /* swap valuestring and string, because we parsed the name */
        current_item->string = current_item->valuestring;
        current_item->valuestring = NULL;
        if (cannot_access_at_index(input_buffer, 0) || (buffer_at_offset(input_buffer)[0] != ':'))
            goto fail; /* invalid object */
        /* parse the value */
        input_buffer->offset++;
        buffer_skip_whitespace(input_buffer);
        if (!parse_value(current_item, input_buffer))
            goto fail; /* failed to parse value */
        buffer_skip_whitespace(input_buffer);
    }
    while (can_access_at_index(input_buffer, 0) && (buffer_at_offset(input_buffer)[0] == ','));

    if (cannot_access_at_index(input_buffer, 0) || (buffer_at_offset(input_buffer)[0] != '}'))
        goto fail; /* expected end of object */
success:
    input_buffer->depth--;
    if (head != NULL)
        head->prev = current_item;
    item->type = X_JSON_OBJECT;
    item->child = head;
    input_buffer->offset++;
    return true;
fail:
    if (head != NULL)
        x_json_delete(head);
    return false;
}

/* Render an object to text. */
static bool print_object(const x_json *const item, printbuffer * const output_buffer)
{
    uint8_t *output_pointer = NULL;
    size_t length = 0;
    x_json *current_item = item->child;
    if (output_buffer == NULL)
        return false;
    /* Compose the output: */
    length = (size_t) (output_buffer->format ? 2 : 1); /* fmt: {\n */
    output_pointer = ensure(output_buffer, length + 1);
    if (!output_pointer)
        return false;
    *output_pointer++ = '{';
    output_buffer->depth++;
    if (output_buffer->format)
        *output_pointer++ = '\n';
    output_buffer->offset += length;
    while (current_item) {
        if (output_buffer->format) {
            size_t i;
            output_pointer = ensure(output_buffer, output_buffer->depth);
            if (!output_pointer) {
                return false;
            }
            for (i = 0; i < output_buffer->depth; i++) {
                *output_pointer++ = '\t';
            }
            output_buffer->offset += output_buffer->depth;
        }
        /* print key */
        if (!print_string_ptr((uint8_t*)current_item->string, output_buffer))
            return false;
        update_offset(output_buffer);

        length = (size_t) (output_buffer->format ? 2 : 1);
        output_pointer = ensure(output_buffer, length);
        if (!output_pointer)
            return false;
        *output_pointer++ = ':';
        if (output_buffer->format)
            *output_pointer++ = '\t';
        output_buffer->offset += length;
        /* print value */
        if (!print_value(current_item, output_buffer))
            return false;
        update_offset(output_buffer);
        /* print comma if not last */
        length = ((size_t)(output_buffer->format ? 1 : 0) + (size_t)(current_item->next ? 1 : 0));
        output_pointer = ensure(output_buffer, length + 1);
        if (!output_pointer)
            return false;
        if (current_item->next)
            *output_pointer++ = ',';
        if (output_buffer->format)
            *output_pointer++ = '\n';
        *output_pointer = '\0';
        output_buffer->offset += length;
        current_item = current_item->next;
    }
    output_pointer = ensure(output_buffer, output_buffer->format ? (output_buffer->depth + 1) : 2);
    if (!output_pointer)
        return false;
    if (output_buffer->format) {
        size_t i;
        for (i = 0; i < (output_buffer->depth - 1); i++)
            *output_pointer++ = '\t';
    }
    *output_pointer++ = '}';
    *output_pointer = '\0';
    output_buffer->depth--;
    return true;
}

/* Get Array size/item / object item. */
int x_json_get_array_size(const x_json *array)
{
    x_json *child = NULL;
    size_t size = 0;
    if (array == NULL)
        return 0;
    child = array->child;
    while(child != NULL) {
        size++;
        child = child->next;
    }
    /* FIXME: Can overflow here. Cannot be fixed without breaking the API */
    return (int)size;
}

static x_json* get_array_item(const x_json *array, size_t index)
{
    x_json *current_child = NULL;
    if (array == NULL)
        return NULL;
    current_child = array->child;
    while ((current_child != NULL) && (index > 0)) {
        index--;
        current_child = current_child->next;
    }
    return current_child;
}

x_json *x_json_get_array_item(const x_json *array, int index)
{
    if (index < 0)
        return NULL;
    return get_array_item(array, (size_t)index);
}

static x_json *get_object_item(const x_json *const object, const char *const name, const bool case_sensitive)
{
    x_json *current_element = NULL;
    if ((object == NULL) || (name == NULL))
        return NULL;
    current_element = object->child;
    if (case_sensitive) {
        while ((current_element != NULL) && (current_element->string != NULL) && (strcmp(name, current_element->string) != 0)) {
            current_element = current_element->next;
        }
    }
    else {
        while ((current_element != NULL) && (case_insensitive_strcmp((const uint8_t*)name, (const uint8_t*)(current_element->string)) != 0)) {
            current_element = current_element->next;
        }
    }
    if ((current_element == NULL) || (current_element->string == NULL)) {
        return NULL;
    }
    return current_element;
}

x_json *x_json_get_object_item(const x_json *const object, const char *const string)
{
    return get_object_item(object, string, false);
}

x_json *x_json_get_object_item_case_sensitive(const x_json *const object, const char *const string)
{
    return get_object_item(object, string, true);
}

bool x_json_has_object_item(const x_json *object, const char *string)
{
    return x_json_get_object_item(object, string) ? 1 : 0;
}

/* Utility for array list handling. */
static void suffix_object(x_json *prev, x_json *item)
{
    prev->next = item;
    item->prev = prev;
}

/* Utility for handling references. */
static x_json *create_reference(const x_json *item)
{
    x_json *reference = NULL;
    if (item == NULL)
        return NULL;
    reference = x_json_new_item();
    if (reference == NULL)
        return NULL;
    memcpy(reference, item, sizeof(x_json));
    reference->string = NULL;
    reference->type |= X_JSON_IS_REF;
    reference->next = reference->prev = NULL;
    return reference;
}

static bool add_item_to_array(x_json *array, x_json *item)
{
    x_json *child = NULL;
    if ((item == NULL) || (array == NULL) || (array == item))
        return false;
    child = array->child;
    /*
     * To find the last item in array quickly, we use prev in array
     */
    if (child == NULL) {
        /* list is empty, start new one */
        array->child = item;
        item->prev = item;
        item->next = NULL;
    }
    else {
        /* append to the end */
        if (child->prev) {
            suffix_object(child->prev, item);
            array->child->prev = item;
        }
    }
    return true;
}

/* Add item to array/object. */
bool x_json_add_item_to_array(x_json *array, x_json *item)
{
    return add_item_to_array(array, item);
}

#if defined(__clang__) || (defined(__GNUC__)  && ((__GNUC__ > 4) || ((__GNUC__ == 4) && (__GNUC_MINOR__ > 5))))
    #pragma GCC diagnostic push
#endif
#ifdef __GNUC__
#pragma GCC diagnostic ignored "-Wcast-qual"
#endif
/* helper function to cast away const */
static void* cast_away_const(const void* string)
{
    return (void*)string;
}
#if defined(__clang__) || (defined(__GNUC__)  && ((__GNUC__ > 4) || ((__GNUC__ == 4) && (__GNUC_MINOR__ > 5))))
    #pragma GCC diagnostic pop
#endif

static bool add_item_to_object(x_json *const object, const char *const string, x_json *const item, const bool constant_key)
{
    char *new_key = NULL;
    int new_type = X_JSON_INVALID;
    if ((object == NULL) || (string == NULL) || (item == NULL) || (object == item))
        return false;
    if (constant_key) {
        new_key = (char*)cast_away_const(string);
        new_type = item->type | X_JSON_STRING_IS_CONST;
    }
    else {
        new_key = (char*)x_json_strdup((const uint8_t*)string);
        new_type = item->type & ~X_JSON_STRING_IS_CONST;
    }
    if (!(item->type & X_JSON_STRING_IS_CONST) && (item->string != NULL))
        x_free(item->string);
    item->string = new_key;
    item->type = new_type;
    return add_item_to_array(object, item);
}

bool x_json_add_item_to_object(x_json *object, const char *string, x_json *item)
{
    return add_item_to_object(object, string, item, false);
}

/* Add an item to an object with constant string as key */
bool x_json_add_item_to_object_cs(x_json *object, const char *string, x_json *item)
{
    return add_item_to_object(object, string, item, true);
}

bool x_json_add_item_reference_to_array(x_json *array, x_json *item)
{
    if (array == NULL)
        return false;
    return add_item_to_array(array, create_reference(item));
}

bool x_json_add_item_reference_to_object(x_json *object, const char *string, x_json *item)
{
    if ((object == NULL) || (string == NULL))
        return false;
    return add_item_to_object(object, string, create_reference(item), false);
}

x_json* x_json_add_null_to_object(x_json *const object, const char *const name)
{
    x_json *null = x_json_create_null();
    if (add_item_to_object(object, name, null, false))
        return null;
    x_json_delete(null);
    return NULL;
}

x_json* x_json_add_true_to_object(x_json *const object, const char *const name)
{
    x_json *true_item = x_json_create_true();
    if (add_item_to_object(object, name, true_item, false))
        return true_item;
    x_json_delete(true_item);
    return NULL;
}

x_json* x_json_add_false_to_object(x_json *const object, const char *const name)
{
    x_json *false_item = x_json_create_false();
    if (add_item_to_object(object, name, false_item, false))
        return false_item;
    x_json_delete(false_item);
    return NULL;
}

x_json* x_json_add_bool_to_object(x_json *const object, const char *const name, const bool boolean)
{
    x_json *bool_item = x_json_create_bool(boolean);
    if (add_item_to_object(object, name, bool_item, false))
        return bool_item;
    x_json_delete(bool_item);
    return NULL;
}

x_json* x_json_add_number_to_object(x_json *const object, const char *const name, const double number)
{
    x_json *number_item = x_json_create_number(number);
    if (add_item_to_object(object, name, number_item, false))
        return number_item;
    x_json_delete(number_item);
    return NULL;
}

x_json* x_json_add_string_to_object(x_json *const object, const char *const name, const char *const string)
{
    x_json *string_item = x_json_create_string(string);
    if (add_item_to_object(object, name, string_item, false))
        return string_item;
    x_json_delete(string_item);
    return NULL;
}

x_json* x_json_add_raw_to_object(x_json *const object, const char *const name, const char *const raw)
{
    x_json *raw_item = x_json_create_raw(raw);
    if (add_item_to_object(object, name, raw_item, false))
        return raw_item;
    x_json_delete(raw_item);
    return NULL;
}

x_json* x_json_add_object_to_object(x_json *const object, const char *const name)
{
    x_json *object_item = x_json_create_object();
    if (add_item_to_object(object, name, object_item, false))
        return object_item;
    x_json_delete(object_item);
    return NULL;
}

x_json* x_json_add_array_to_object(x_json *const object, const char *const name)
{
    x_json *array = x_json_create_array();
    if (add_item_to_object(object, name, array, false))
        return array;
    x_json_delete(array);
    return NULL;
}

x_json *x_json_detach_item_via_pointer(x_json *parent, x_json *const item)
{
    if ((parent == NULL) || (item == NULL) || (item != parent->child && item->prev == NULL))
        return NULL;
    if (item != parent->child) /* not the first element */
        item->prev->next = item->next;
    if (item->next != NULL) /* not the last element */
        item->next->prev = item->prev;
    if (item == parent->child) /* first element */
        parent->child = item->next;
    else if (item->next == NULL) /* last element */
        parent->child->prev = item->prev;
    /* make sure the detached item doesn't point anywhere anymore */
    item->prev = NULL;
    item->next = NULL;
    return item;
}

x_json *x_json_detach_item_from_array(x_json *array, int which)
{
    if (which < 0)
        return NULL;
    return x_json_detach_item_via_pointer(array, get_array_item(array, (size_t)which));
}

void x_json_delete_item_from_array(x_json *array, int which)
{
    x_json_delete(x_json_detach_item_from_array(array, which));
}

x_json *x_json_detach_item_from_object(x_json *object, const char *string)
{
    x_json *to_detach = x_json_get_object_item(object, string);
    return x_json_detach_item_via_pointer(object, to_detach);
}

x_json *x_json_detach_item_from_object_case_sensitive(x_json *object, const char *string)
{
    x_json *to_detach = x_json_get_object_item_case_sensitive(object, string);
    return x_json_detach_item_via_pointer(object, to_detach);
}

void x_json_delete_item_from_object(x_json *object, const char *string)
{
    x_json_delete(x_json_detach_item_from_object(object, string));
}

void x_json_delete_item_from_object_case_sensitive(x_json *object, const char *string)
{
    x_json_delete(x_json_detach_item_from_object_case_sensitive(object, string));
}

/* Replace array/object items with new ones. */
bool x_json_insert_item_in_array(x_json *array, int which, x_json *newitem)
{
    x_json *after_inserted = NULL;
    if (which < 0 || newitem == NULL)
        return false;
    after_inserted = get_array_item(array, (size_t)which);
    if (after_inserted == NULL)
        return add_item_to_array(array, newitem);
    if (after_inserted != array->child && after_inserted->prev == NULL)
        /* return false if after_inserted is a corrupted array item */
        return false;
    newitem->next = after_inserted;
    newitem->prev = after_inserted->prev;
    after_inserted->prev = newitem;
    if (after_inserted == array->child)
        array->child = newitem;
    else
        newitem->prev->next = newitem;
    return true;
}

void x_json_replace_item_via_pointer(x_json *const parent, x_json *const item, x_json *replacement)
{
    assert(parent&& parent->child && replacement && item);
    if (replacement == item)
		return;
    replacement->next = item->next;
    replacement->prev = item->prev;
    if (replacement->next != NULL)
        replacement->next->prev = replacement;
    if (parent->child == item) {
        if (parent->child->prev == parent->child)
            replacement->prev = replacement;
        parent->child = replacement;
    }
    else {
		/*
         * To find the last item in array quickly, we use prev in array.
         * We can't modify the last item's next pointer where this item was the parent's child
         */
        if (replacement->prev != NULL)
            replacement->prev->next = replacement;
        if (replacement->next == NULL)
            parent->child->prev = replacement;
    }
    item->next = NULL;
    item->prev = NULL;
    x_json_delete(item);
}

void x_json_replace_item_in_array(x_json *array, int which, x_json *newitem)
{
    if (which < 0)
		return;
    x_json_replace_item_via_pointer(array, get_array_item(array, (size_t)which), newitem);
}

static void replace_item_in_object(x_json *object, const char *string, x_json *replacement, bool case_sensitive)
{
    assert ((replacement != NULL) || (string != NULL));
    /* replace the name in the replacement */
    if (!(replacement->type & X_JSON_STRING_IS_CONST) && (replacement->string != NULL)) {
        x_json_free(replacement->string);
    }
    replacement->string = (char*)x_json_strdup((const uint8_t*)string);
    replacement->type &= ~X_JSON_STRING_IS_CONST;
    x_json_replace_item_via_pointer(object, get_object_item(object, string, case_sensitive), replacement);
}

void x_json_replace_item_in_object(x_json *object, const char *string, x_json *newitem)
{
    replace_item_in_object(object, string, newitem, false);
}

void x_json_replace_item_in_object_case_sensitive(x_json *object, const char *string, x_json *newitem)
{
    replace_item_in_object(object, string, newitem, true);
}

x_json *x_json_create_null(void)
{
    x_json *item = x_json_new_item();
	item->type = x_json_NULL;
    return item;
}

x_json *x_json_create_true(void)
{
    x_json *item = x_json_new_item();
	item->type = X_JSON_TRUE;
    return item;
}

x_json *x_json_create_false(void)
{
    x_json *item = x_json_new_item();
	item->type = X_JSON_FALSE;
    return item;
}

x_json *x_json_create_bool(bool boolean)
{
    x_json *item = x_json_new_item();
	item->type = boolean ? X_JSON_TRUE : X_JSON_FALSE;
    return item;
}

x_json *x_json_create_number(double num)
{
    x_json *item = x_json_new_item();
	item->type = X_JSON_NUMBER;
	item->valuedouble = num;
	/* use saturation in case of overflow */
	if (num >= INT_MAX)
		item->valueint = INT_MAX;
	else if (num <= (double)INT_MIN)
		item->valueint = INT_MIN;
	else
		item->valueint = (int)num;
    return item;
}

x_json *x_json_create_string(const char *string)
{
	x_json *item = x_json_new_item();
	item->type = X_JSON_STRING;
	item->valuestring = (char*)x_json_strdup((const uint8_t*)string);
	if(!item->valuestring) {
		x_json_delete(item);
		return NULL;
	}
	return item;
}

x_json *x_json_create_string_reference(const char *string)
{
    x_json *item = x_json_new_item();
	item->type = X_JSON_STRING | X_JSON_IS_REF;
	item->valuestring = (char*)cast_away_const(string);
    return item;
}

x_json *x_json_create_object_reference(const x_json *child)
{
    x_json *item = x_json_new_item();
	item->type = X_JSON_OBJECT | X_JSON_IS_REF;
	item->child = (x_json*)cast_away_const(child);
    return item;
}

x_json *x_json_create_array_reference(const x_json *child)
{
    x_json *item = x_json_new_item();
	item->type = X_JSON_ARRAY | X_JSON_IS_REF;
	item->child = (x_json*)cast_away_const(child);
    return item;
}

x_json *x_json_create_raw(const char *raw)
{
	x_json *item = x_json_new_item();
	item->type = X_JSON_RAW;
	item->valuestring = (char*)x_json_strdup((const uint8_t*)raw);
	if(!item->valuestring) {
		x_json_delete(item);
		return NULL;
	}
    return item;
}

x_json *x_json_create_array(void)
{
    x_json *item = x_json_new_item();
	item->type=X_JSON_ARRAY;
    return item;
}

x_json *x_json_create_object(void)
{
    x_json *item = x_json_new_item();
	item->type = X_JSON_OBJECT;
    return item;
}

/* Create Arrays: */
x_json *x_json_create_int_array(const int *numbers, int count)
{
    size_t i = 0;
    x_json *n = NULL;
    x_json *p = NULL;
    x_json *a = NULL;
    if ((count < 0) || (numbers == NULL))
        return NULL;
    a = x_json_create_array();
    for(i = 0; a && (i < (size_t)count); i++) {
        n = x_json_create_number(numbers[i]);
        if(!i)
            a->child = n;
        else
            suffix_object(p, n);
        p = n;
    }
    if (a && a->child)
        a->child->prev = n;
    return a;
}

x_json *x_json_create_float_array(const float *numbers, int count)
{
    size_t i = 0;
    x_json *n = NULL, *p = NULL, *a = NULL;
    if ((count < 0) || (numbers == NULL))
        return NULL;
    a = x_json_create_array();
    for(i = 0; a && (i < (size_t)count); i++) {
        n = x_json_create_number((double)numbers[i]);
        if(!i)
            a->child = n;
        else
            suffix_object(p, n);
        p = n;
    }
    if (a && a->child)
        a->child->prev = n;
    return a;
}

x_json *x_json_create_double_array(const double *numbers, int count)
{
    size_t i = 0;
    x_json *n = NULL, *p = NULL, *a = NULL;
    if ((count < 0) || (numbers == NULL))
        return NULL;
    a = x_json_create_array();
    for(i = 0; a && (i < (size_t)count); i++) {
        n = x_json_create_number(numbers[i]);
        if(!i)
            a->child = n;
        else
            suffix_object(p, n);
        p = n;
    }
    if (a && a->child)
        a->child->prev = n;
    return a;
}

x_json *x_json_create_string_array(const char *const *strings, int count)
{
    size_t i = 0;
    x_json *n = NULL, *p = NULL, *a = NULL;
    if ((count < 0) || (strings == NULL))
        return NULL;
    a = x_json_create_array();
    for (i = 0; a && (i < (size_t)count); i++) {
        n = x_json_create_string(strings[i]);
        if(!n) {
            x_json_delete(a);
            return NULL;
        }
        if(!i)
            a->child = n;
        else
            suffix_object(p,n);
        p = n;
    }
    if (a && a->child)
        a->child->prev = n;
    return a;
}

/* Duplication */
x_json *x_json_duplicate_rec(const x_json *item, size_t depth, bool recurse);

x_json *x_json_duplicate(const x_json *item, bool recurse)
{
    return x_json_duplicate_rec(item, 0, recurse );
}

x_json *x_json_duplicate_rec(const x_json *item, size_t depth, bool recurse)
{
    x_json *newitem = NULL;
    x_json *child = NULL;
    x_json *next = NULL;
    x_json *newchild = NULL;
    /* Bail on bad ptr */
    if (!item) {
        goto fail;
    }
    /* Create new item */
    newitem = x_json_new_item();
    if (!newitem) {
        goto fail;
    }
    /* Copy over all vars */
    newitem->type = item->type & (~X_JSON_IS_REF);
    newitem->valueint = item->valueint;
    newitem->valuedouble = item->valuedouble;
    if (item->valuestring) {
        newitem->valuestring = (char*)x_json_strdup((uint8_t*)item->valuestring);
        if (!newitem->valuestring) {
            goto fail;
        }
    }
    if (item->string) {
        newitem->string = (item->type&X_JSON_STRING_IS_CONST) ? item->string : (char*)x_json_strdup((uint8_t*)item->string);
        if (!newitem->string) {
            goto fail;
        }
    }
    /* If non-recursive, then we're done! */
    if (!recurse)
        return newitem;
    /* Walk the ->next chain for the child. */
    child = item->child;
    while (child != NULL) {
        if(depth >= X_JSON_CIRCULAR_LIMIT)
            goto fail;
        newchild = x_json_duplicate_rec(child, depth + 1, true); /* Duplicate (with recurse) each item in the ->next chain */
        if (!newchild)
            goto fail;
        if (next != NULL) {
            /* If newitem->child already set, then crosswire ->prev and ->next and move on */
            next->next = newchild;
            newchild->prev = next;
            next = newchild;
        }
        else {
            /* Set newitem->child and move to it */
            newitem->child = newchild;
            next = newchild;
        }
        child = child->next;
    }
    if (newitem && newitem->child)
        newitem->child->prev = newchild;
    return newitem;
fail:
    if (newitem != NULL)
        x_json_delete(newitem);
    return NULL;
}

static void skip_oneline_comment(char **input)
{
    *input += static_strlen("//");
    for (; (*input)[0] != '\0'; ++(*input)) {
        if ((*input)[0] == '\n') {
            *input += static_strlen("\n");
            return;
        }
    }
}

static void skip_multiline_comment(char **input)
{
    *input += static_strlen("/*");
    for (; (*input)[0] != '\0'; ++(*input)) {
        if (((*input)[0] == '*') && ((*input)[1] == '/')) {
            *input += static_strlen("*/");
            return;
        }
    }
}

static void minify_string(char **input, char **output) {
    (*output)[0] = (*input)[0];
    *input += static_strlen("\"");
    *output += static_strlen("\"");
    for (; (*input)[0] != '\0'; (void)++(*input), ++(*output)) {
        (*output)[0] = (*input)[0];
        if ((*input)[0] == '\"') {
            (*output)[0] = '\"';
            *input += static_strlen("\"");
            *output += static_strlen("\"");
            return;
        } else if (((*input)[0] == '\\') && ((*input)[1] == '\"')) {
            (*output)[1] = (*input)[1];
            *input += static_strlen("\"");
            *output += static_strlen("\"");
        }
    }
}

void x_json_minify(char *json)
{
    char *into = json;
    if (!json)
        return;
    while (json[0] != '\0') {
        switch (json[0]) {
            case ' ':
            case '\t':
            case '\r':
            case '\n':
                json++;
                break;
            case '/':
                if (json[1] == '/')
                    skip_oneline_comment(&json);
                else if (json[1] == '*')
                    skip_multiline_comment(&json);
                else
                    json++;
                break;
            case '\"':
                minify_string(&json, (char**)&into);
                break;
            default:
                into[0] = json[0];
                json++;
                into++;
        }
    }
    /* and null-terminate. */
    *into = '\0';
}

bool x_json_is_invalid(const x_json *const item)
{
	if (!item)
		return false;
	return (item->type & 0xFF) == X_JSON_INVALID;
}

bool x_json_is_false(const x_json *const item)
{
	if (!item)
		return false;
	return (item->type & 0xFF) == X_JSON_FALSE;
}

bool x_json_is_true(const x_json *const item)
{
	if (!item)
		return false;
	return (item->type & 0xff) == X_JSON_TRUE;
}

bool x_json_is_bool(const x_json *const item)
{
	if (!item)
		return false;
	return (item->type & (X_JSON_TRUE | X_JSON_FALSE)) != 0;
}
bool x_json_is_null(const x_json *const item)
{
	if (!item)
		return false;
	return (item->type & 0xFF) == x_json_NULL;
}

bool x_json_is_number(const x_json *const item)
{
	if (!item)
		return false;
	return (item->type & 0xFF) == X_JSON_NUMBER;
}

bool x_json_is_string(const x_json *const item)
{
	if (!item)
		return false;
	return (item->type & 0xFF) == X_JSON_STRING;
}

bool x_json_is_array(const x_json *const item)
{
	if (!item)
		return false;
	return (item->type & 0xFF) == X_JSON_ARRAY;
}

bool x_json_is_object(const x_json *const item)
{
	if (!item)
		return false;
	return (item->type & 0xFF) == X_JSON_OBJECT;
}

bool x_json_is_raw(const x_json *const item)
{
	if (!item)
		return false;
	return (item->type & 0xFF) == X_JSON_RAW;
}

bool x_json_compare(const x_json *const a, const x_json *const b, const bool case_sensitive)
{
    if ((a == NULL) || (b == NULL) || ((a->type & 0xFF) != (b->type & 0xFF)))
        return false;
    /* check if type is valid */
    switch (a->type & 0xFF) {
        case X_JSON_FALSE:
        case X_JSON_TRUE:
        case x_json_NULL:
        case X_JSON_NUMBER:
        case X_JSON_STRING:
        case X_JSON_RAW:
        case X_JSON_ARRAY:
        case X_JSON_OBJECT:
            break;
        default:
            return false;
    }
    /* identical objects are equal */
    if (a == b)
        return true;
    switch (a->type & 0xFF) {
        /* in these cases and equal type is enough */
        case X_JSON_FALSE:
        case X_JSON_TRUE:
        case x_json_NULL:
            return true;

        case X_JSON_NUMBER:
            if (compare_double(a->valuedouble, b->valuedouble))
                return true;
            return false;
        case X_JSON_STRING:
        case X_JSON_RAW:
            if ((a->valuestring == NULL) || (b->valuestring == NULL))
                return false;
            if (strcmp(a->valuestring, b->valuestring) == 0)
                return true;
            return false;
        case X_JSON_ARRAY:
        {
            x_json *a_element = a->child;
            x_json *b_element = b->child;

            for (; (a_element != NULL) && (b_element != NULL);) {
                if (!x_json_compare(a_element, b_element, case_sensitive))
                    return false;
                a_element = a_element->next;
                b_element = b_element->next;
            }
            /* one of the arrays is longer than the other */
            if (a_element != b_element)
                return false;
            return true;
        }
        case X_JSON_OBJECT:
        {
            x_json *a_element = NULL;
            x_json *b_element = NULL;
            x_json_array_foreach(a_element, a) {
                /* TODO This has O(n^2) runtime, which is horrible! */
                b_element = get_object_item(b, a_element->string, case_sensitive);
                if (b_element == NULL)
                    return false;
                if (!x_json_compare(a_element, b_element, case_sensitive))
                    return false;
            }
            /* doing this twice, once on a and b to prevent true comparison if a subset of b
             * TODO: Do this the proper way, this is just a fix for now */
            x_json_array_foreach(b_element, b) {
                a_element = get_object_item(a, b_element->string, case_sensitive);
                if (a_element == NULL)
                    return false;
                if (!x_json_compare(b_element, a_element, case_sensitive))
                    return false;
            }
            return true;
        }
        default:
            return false;
    }
}

void * x_json_malloc(size_t size)
{
    return x_malloc(NULL, size);
}

void x_json_free(void *object)
{
    x_free(object);
    object = NULL;
}

