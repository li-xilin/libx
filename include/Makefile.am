xincludedir = $(includedir)/x
xinclude_HEADERS = \
	x/assert.h \
	x/base64.h \
	x/bitmap.h \
	x/cond.h \
	x/tss.h \
	x/macros.h \
	x/detect.h \
	x/dump.h \
	x/flowctl.h \
	x/heap.h \
	x/ini.h \
	x/list.h \
	x/log.h \
	x/memory.h \
	x/mutex.h \
	x/narg.h \
	x/once.h \
	x/pipe.h \
	x/splay.h \
	x/string.h \
	x/tcolor.h \
	x/thread.h \
	x/tpool.h \
	x/trick.h \
	x/rope.h \
	x/btnode.h \
	x/types.h \
	x/unicode.h \
	x/test.h \
	x/file.h \
	x/uchar.h \
	x/tsignal.h \
	x/dir.h \
	x/stat.h \
	x/proc.h \
	x/cliarg.h \
	x/sys.h \
	x/path.h \
	x/edit.h \
	x/strbuf.h \
	x/errno.h

nodist_xinclude_HEADERS = \
	x/version.h

x/version.h: x/version.h.in $(top_builddir)/config.status
	$(AM_V_GEN)$(SED) -e 's|@VERSION_MAJOR[@]|$(VERSION_MAJOR)|g' \
	                  -e 's|@VERSION_MINOR[@]|$(VERSION_MINOR)|g' \
	                  -e 's|@VERSION_PATCH[@]|$(VERSION_PATCH)|g' \
	                  $< > $@
CLEANFILES = x/version.h
DISTCLEANFILES = Makefile.in
EXTRA_DIST = x/version.h.in

dist-hook:
	$(RM) $(distdir)/Makefile.in
