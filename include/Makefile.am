xincludedir = $(includedir)/x
xinclude_HEADERS = \
	x/version.h \
	x/compiler.h \
	x/linkage.h \
	x/assert.h \
	x/base64.h \
	x/bitmap.h \
	x/cond.h \
	x/tss.h \
	x/macros.h \
	x/detect.h \
	x/endian.h \
	x/dump.h \
	x/flowctl.h \
	x/heap.h \
	x/ini.h \
	x/list.h \
	x/array.h \
	x/log.h \
	x/memory.h \
	x/arena.h \
	x/mutex.h \
	x/rwlock.h \
	x/narg.h \
	x/once.h \
	x/pipe.h \
	x/splay.h \
	x/string.h \
	x/tcolor.h \
	x/thread.h \
	x/thrpool.h \
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
	x/lib.h \
	x/cliarg.h \
	x/sys.h \
	x/path.h \
	x/strbuf.h \
	x/printf.h \
	x/hmap.h \
	x/time.h \
	x/future.h \
	x/twister.h \
	x/index.h \
	x/pathset.h \
	x/fwalker.h \
	x/guard.h \
	x/debug.h \
	x/hfsm.h \
	x/sklist.h \
	x/uidpool.h \
	x/ringbuf.h \
	x/deqbuf.h \
	x/uuid.h \
	x/objpool.h \
	x/errno.h

nodist_xinclude_HEADERS = \
	x/version.h

if ENABLE_NETWORK
xinclude_HEADERS +=  x/event.h x/reactor.h x/socket.h x/asynet.h x/ipcsock.h x/invoke.h
endif

if ENABLE_JSON
xinclude_HEADERS +=  x/json.h x/jpath.h
endif

if ENABLE_EDIT
xinclude_HEADERS +=  x/edit.h
endif

if ENABLE_REGEX
xinclude_HEADERS +=  x/regex.h
endif

if ENABLE_CRYPTO
xinclude_HEADERS +=  x/aes.h x/sha256.h x/random.h x/md5.h
endif

if ENABLE_CHARMAP
xinclude_HEADERS +=  x/charmap.h
endif

BUILT_SOURCES = x/version.h
x/version.h: x/version.h.in $(top_builddir)/config.status
	$(MKDIR_P) x
	$(AM_V_GEN)$(SED) -e 's|@VERSION_MAJOR[@]|$(VERSION_MAJOR)|g' \
	                  -e 's|@VERSION_MINOR[@]|$(VERSION_MINOR)|g' \
	                  -e 's|@VERSION_PATCH[@]|$(VERSION_PATCH)|g' \
	                  $< > $@
EXTRA_DIST = x/version.h.in
MAINTAINERCLEANFILES = Makefile.in
