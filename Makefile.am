ACLOCAL_AMFLAGS = -I m4
SUBDIRS = libx include samples
DIST_SUBDIRS = $(SUBDIRS)

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = libx.pc

${pkgconfig_DATA}: ${top_builddir}/config.status

