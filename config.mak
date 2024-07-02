# Copyright (c) 2020-2024 Li Xilin <lixilin@gmx.com>
# 
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
# 
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
# 
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
# THE SOFTWARE.

AR            = ar
RM            = rm -f
CC            = gcc
CFLAGS        = 
LDFLAGS       = 

INCLUDE = $(ROOT)/include
LIB = $(ROOT)/lib
BIN = $(ROOT)/bin

CFLAGS += --pedantic -std=c99 -I$(ROOT)/src/include -I$(INCLUDE)
CFLAGS += -Wall -Werror -fPIC -D_POSIX_C_SOURCE=199309L

DISABLE_DEBUG = no
DISABLE_CASSERT = no

ifeq ($(DISABLE_DEBUG),yes)
	CFLAGS += -O2
else
	CFLAGS += -g -O0
endif

ifeq ($(DISABLE_CASSERT),yes)
	CFLAGS += -DNDEBUG
endif

