/**
 * resizable string buffer
 * Copyright (c) 2017-2020 Steve Bennett <steveb@workware.net.au>
 * Copyright (c) 2023,2025 Li Xilin <lixilin@gmx.com>
 */

#ifndef X_STRBUF_H
#define X_STRBUF_H

#include "types.h"
#include "uchar.h"

x_strbuf *x_strbuf_alloc(void);
void x_strbuf_free(x_strbuf *sb);
x_strbuf *x_strbuf_copy(x_strbuf *sb);
int x_strbuf_len(x_strbuf *sb);
int x_strbuf_chars(x_strbuf *sb);
void x_strbuf_append(x_strbuf *sb, const x_uchar *str);
void x_strbuf_append_len(x_strbuf *sb, const x_uchar *str, int len);
x_uchar *x_strbuf_str(const x_strbuf *sb);
void x_strbuf_insert(x_strbuf *sb, int index, const x_uchar *str);
/**
 * If len is -1, deletes to the end of the buffer.
 */
void x_strbuf_delete(x_strbuf *sb, int index, int len);
void x_strbuf_clear(x_strbuf *sb);
x_uchar *x_strbuf_to_string(x_strbuf *sb);

#endif
