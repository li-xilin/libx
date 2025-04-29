#!/bin/sh -x

make distclean
rm -rf autom4te.cache build-aux m4 libx/.deps libx/.libs samples/.deps
rm -f Makefile.in aclocal.m4 config.h.in config.h.in~ configure \
	configure~ config.h config.status config.log libtool \
	stamp-h1 libx.pc include/Makefile.in src/Makefile.in
[ ! -f configure.ac ] && rmdir include libx samples
