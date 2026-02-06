lib_LTLIBRARIES = libx.la

AM_CFLAGS = $(regular_CFLAGS) -fPIC -I$(top_srcdir)/include -Wno-format-nonliteral -Wno-strict-aliasing -DX_SUPPORT_PRINTF_IXX=1
libx_la_LDFLAGS = -Wl,--version-script=$(srcdir)/libx.map -version-info $(LT_VERSION_INFO) -no-undefined
libx_la_SOURCES = assert.c base64.c bitmap.c dump.c dumpfmt.c heap.c ini.c log.c \
		memory.c pipe.c splay.c string.c tcolor.c rope.c btnode.c tpool.c errno.c \
		tss.c thread.c once.c mutex.c rwlock.c cond.c unicode.c test.c uchar.c file.c \
		strbuf.c tsignal.c dir.c stat.c proc.c cliarg.c sys.c path.c printf.c hmap.c \
		time.c lib.c future.c twister.c index.c pathset.c fwalker.c

if ENABLE_NETWORK
if WINDOWS
libx_la_LDFLAGS += -lshlwapi -lws2_32
libx_la_SOURCES += wepoll.c
else
libx_la_LDFLAGS += -ldl
endif
libx_la_SOURCES +=  event.c reactor.c mux_epoll.c socket.c
endif

if ENABLE_JSON
libx_la_SOURCES +=  json.c jpath.c
endif

if ENABLE_EDIT
libx_la_SOURCES +=  edit.c
endif

if ENABLE_REGEX
libx_la_SOURCES +=  regex.c
endif

if ENABLE_CRYPTO
libx_la_SOURCES +=  aes.c sha256.c random.c
endif

if ENABLE_CHARMAP
libx_la_SOURCES +=  charmap.c charmap_big5.c  charmap_euckr.c  charmap_gbk.c  charmap_sjis.c
endif

DISTCLEANFILES = Makefile.in
EXTRA_DIST = libx.map strargv.inc wepoll.h
dist-hook:
	$(RM) $(distdir)/Makefile.in

