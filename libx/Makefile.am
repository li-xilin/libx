lib_LTLIBRARIES = libx.la

AM_CFLAGS = $(regular_CFLAGS) -fPIC -I$(top_srcdir)/include -Wno-format-nonliteral -Wno-strict-aliasing
libx_la_LDFLAGS = -Wl,--version-script=$(srcdir)/libx.map -version-info $(LT_VERSION_INFO) -no-undefined
libx_la_SOURCES = assert.c base64.c bitmap.c dump.c dumpfmt.c heap.c ini.c log.c memory.c pipe.c splay.c string.c tcolor.c rope.c btnode.c tpool.c errno.c tss.c thread.c once.c mutex.c cond.c unicode.c test.c uchar.c file.c edit.c strbuf.c tsignal.c dir.c stat.c proc.c cliarg.c sys.c path.c printf.c hmap.c time.c event.c reactor.c mux_epoll.c socket.c
if WINDOWS
libx_la_LDFLAGS += -lshlwapi -lws2_32
libx_la_SOURCES += wepoll.c
endif
DISTCLEANFILES = Makefile.in
EXTRA_DIST = libx.map strargv.inc wepoll.h
dist-hook:
	$(RM) $(distdir)/Makefile.in

