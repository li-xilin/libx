ACLOCAL_AMFLAGS = -I m4
SUBDIRS = libx include samples build-aux
DIST_SUBDIRS = $(SUBDIRS)
pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = libx.pc
${pkgconfig_DATA}: ${top_builddir}/config.status
EXTRA_DIST = LICENSE autogen.sh
DISTCLEANFILES = configure m4 config.h.in aclocal.m4 Makefile.in configure~
distclean-local:
	$(RM) -r $(builddir)/autom4te.cache 
	$(RM) -r $(builddir)/m4
dist-hook:
	$(RM) $(distdir)/configure
	$(RM) $(distdir)/config.h.in
	$(RM) $(distdir)/Makefile.in
	$(RM) $(distdir)/aclocal.m4
	$(RM) -r $(distdir)/m4

