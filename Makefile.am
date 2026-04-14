ACLOCAL_AMFLAGS = -I m4
SUBDIRS = include libx samples test
DIST_SUBDIRS = $(SUBDIRS)
pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = libx.pc
${pkgconfig_DATA}: ${top_builddir}/config.status
EXTRA_DIST = LICENSE README.txt README_zh.txt
MAINTAINERCLEANFILES = aclocal.m4 config.h.in~ configure configure~ Makefile.in

maintainer-clean-local:
	$(RM) -r build-aux m4

