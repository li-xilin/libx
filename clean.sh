#!/bin/sh -x
make distclean
rm -rf autom4te.cache/
rm -rf build-aux/
rm -rf m4/
rm -rf src/.deps/
rm -rf src/.libs/
rm -f Makefile.in
rm -f aclocal.m4
rm -f config.h.in
rm -f config.h.in~
rm -f configure
rm -f configure~
rm -f config.h
rm -f config.status
rm -f config.log
rm -f libtool
rm -f stamp-h1
rm -f libx.pc
rm -f include/Makefile.in
rm -f src/Makefile.in

