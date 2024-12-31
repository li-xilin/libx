# Copyright (c) 2024 Li Xilin <lixilin@gmx.com>
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

DESTDIR ?= /usr/local
ALL_CFLAGS += --pedantic -std=c99 -I ../include -Wall -Werror \
	      -fPIC -D_POSIX_C_SOURCE=199309L -D_WIN32_WINNT=0x0600

ifeq ($(CFLAGS),)
	CFLAGS = -g -O2
endif

all:
	$(MAKE) -C src $@ CFLAGS="$(ALL_CFLAGS) $(CFLAGS)" CC=$(CC) AR=$(AR)
clean:
	$(MAKE) -C src $@

install: all
	install -m 755 -d $(DESTDIR)/include/x $(DESTDIR)/lib
	install -m 644 src/libx.a $(DESTDIR)/lib
	install -m 644 include/x/*.h $(DESTDIR)/include/x

.PHONY: all clean install

