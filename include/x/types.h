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

#ifndef X_TYPES_H
#define X_TYPES_H

#include "detect.h"
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <stdarg.h>
#include <limits.h>

#ifdef X_OS_WIN
typedef struct { uint8_t fileid[16]; } x_ino;
typedef uint64_t x_dev;
typedef uint32_t x_pid;
#else
#include <sys/types.h>
typedef ino_t x_ino;
typedef dev_t x_dev;
typedef pid_t x_pid;
#endif

#if SIZE_MAX == UINT16_MAX
typedef int16_t x_ssize ;
#elif SIZE_MAX == UINT32_MAX
typedef int32_t x_ssize ;
#elif SIZE_MAX == UINT64_MAX
typedef int64_t x_ssize ;
#else
#error "Unknown the size of size_t"
#endif


#ifndef X_ROPE_NODE_DEFINED
#define X_ROPE_NODE_DEFINED
typedef struct x_rope_node_st x_rope_node;
#endif

#ifndef X_ROPE_DEFINED
#define X_ROPE_DEFINED
typedef struct x_rope_st x_rope;
#endif

#ifndef X_SPLAY_DEFINED
#define X_SPLAY_DEFINED
typedef struct x_splay_st x_splay;
#endif

#ifndef X_TCOLOR_DEFINED
#define X_TCOLOR_DEFINED
typedef struct x_tcolor_st x_tcolor;
#endif

#ifndef X_THREAD_DEFINED
#define X_THREAD_DEFINED
typedef struct x_thread_st x_thread;
#endif

#ifndef X_TSS_DEFINED
#define X_TSS_DEFINED
typedef struct x_tss_st x_tss;
#endif

#ifndef X_PIPE_DEFINED
#define X_PIPE_DEFINED
typedef struct x_pipe_st x_pipe;
#endif

#ifndef X_ONCE_DEFINED
#define X_ONCE_DEFINED
typedef struct x_once_st x_once;
#endif

#ifndef X_MSET_DEFINED
#define X_MSET_DEFINED
typedef struct x_mset_st x_mset;
#endif

#ifndef X_LINK_DEFINED
#define X_LINK_DEFINED
typedef struct x_link_st x_link;
#endif

#ifndef X_LIST_DEFINED
#define X_LIST_DEFINED
typedef struct x_list_st x_list;
#endif

#ifndef X_INI_DEFINED
#define X_INI_DEFINED
typedef struct x_ini_st x_ini;
#endif

#ifndef X_INI_SECTION_DEFINED
#define X_INI_SECTION_DEFINED
typedef struct x_ini_section_st x_ini_section;
#endif

#ifndef X_INI_OPTION_DEFINED
#define X_INI_OPTION_DEFINED
typedef struct x_ini_option_st x_ini_option;
#endif

#ifndef X_HEAP_DEFINED
#define X_HEAP_DEFINED
typedef struct x_heap_st x_heap;
#endif

#ifndef X_DUMP_DEFINED
#define X_DUMP_DEFINED
typedef struct x_dump_st x_dump;
#endif

#ifndef X_DUMP_FORMAT_DEFINED
#define X_DUMP_FORMAT_DEFINED
typedef struct x_dump_format_st x_dump_format;
#endif

#ifndef X_COND_DEFINED
#define X_COND_DEFINED
typedef struct x_cond_st x_cond;
#endif

#ifndef X_BTNODE_DEFINED
#define X_BTNODE_DEFINED
typedef struct x_btnode_st x_btnode;
#endif

#ifndef X_BITMAP_DEFINED
#define X_BITMAP_DEFINED
typedef struct x_bitmap_st x_bitmap;
#endif

#ifndef X_TPOOL_DEFINED
#define X_TPOOL_DEFINED
typedef struct x_tpool_st x_tpool;
#endif

#ifndef X_TPOOL_WORK_DEFINED
#define X_TPOOL_WORK_DEFINED
typedef struct x_tpool_work_st x_tpool_work;
#endif

#ifndef X_ONCE_DEFINED
#define X_ONCE_DEFINED
typedef struct x_once_st x_once;
#endif

#ifndef X_ONCE_FN_DEFINED
#define X_ONCE_FN_DEFINED
typedef void x_once_fn(void);
#endif

#ifndef X_MUTEX_DEFINED
#define X_MUTEX_DEFINED
typedef struct x_mutex_st x_mutex;
#endif

#ifndef X_STRBUF_DEFINED
#define X_STRBUF_DEFINED
typedef struct x_strbuf_st x_strbuf;
#endif

#ifndef X_CLIARG_DEFINED
#define X_CLIARG_DEFINED
typedef struct x_cliarg_st x_cliarg;
#endif

#ifndef X_CLIARG_LONG_DEFINED
#define X_CLIARG_LONG_DEFINED
typedef struct x_cliarg_long_st x_cliarg_long;
#endif

#ifndef X_FILE_DEFINED
#define X_FILE_DEFINED
typedef struct x_file_st x_file;
#endif

#ifndef X_HMAP_DEFINED
#define X_HMAP_DEFINED
typedef struct x_hmap_st x_hmap;
#endif

#endif

