/*
 * Copyright (c) 2019 win32ports
 * Copyright (c) 2023 Li xilin <lixilin@gmx.com>
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

#ifndef X_DIR_H
#define X_DIR_H

#include "types.h"
#include "detect.h"
#include "uchar.h"
#include <stdint.h>

#ifndef X_OS_WIN
#  include <dirent.h>
#  define  X_DT_UNKNOWN DT_UNKNOWN
#  define  X_DT_FIFO DT_FIFO
#  define  X_DT_CHR  DT_CHR
#  define  X_DT_DIR  DT_DIR
#  define  X_DT_BLK  DT_BLK
#  define  X_DT_REG  DT_REG
#  define  X_DT_LNK  DT_LNK
#  define  X_DT_SOCK DT_SOCK
#  define  X_DT_WHT  DT_WHT
typedef struct dirent x_dirent; 
typedef DIR x_dir;
#else
#  include <sys/types.h>
#  define X_DT_FIFO 1
#  define X_DT_CHR 2
#  define X_DT_DIR 4
#  define X_DT_BLK 6
#  define X_DT_REG 8
#  define X_DT_LNK 10
#  define X_DT_SOCK 12
#  define X_DT_WHT 14
typedef struct __x_dirent_st {
	x_ino d_ino;
	off_t d_off;
	unsigned short d_reclen;
	unsigned char d_type;
	wchar_t d_name[260];
} x_dirent;
typedef struct __x_dir_st x_dir;
#endif

x_dir *x_dir_open(const x_uchar *path);
x_dirent *x_dir_read(x_dir *dir);
void x_dir_close(x_dir *dir);

#endif
