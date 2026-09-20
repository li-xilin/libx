lib_LTLIBRARIES = libx.la

AM_CFLAGS = $(regular_CFLAGS) -fPIC -I$(top_srcdir)/include -Wno-format-nonliteral \
		-Wno-array-bounds -Wno-strict-aliasing -DX_SUPPORT_PRINTF_IXX=1
libx_la_LDFLAGS = -version-info $(LT_VERSION_INFO) -no-undefined
libx_la_SOURCES = assert.c base64.c bitmap.c dump.c dumpfmt.c heap.c ini.c log.c \
		memory.c pipe.c splay.c string.c tcolor.c rope.c btnode.c thrpool.c errno.c \
		tss.c thread.c once.c mutex.c rwlock.c cond.c unicode.c test.c uchar.c file.c \
		strbuf.c tsignal.c dir.c stat.c proc.c cliarg.c sys.c path.c printf.c hmap.c \
		time.c lib.c future.c twister.c index.c pathset.c fwalker.c hash.c debug.c \
		hfsm.c list.c sklist.c uidpool.c ringbuf.c deqbuf.c uuid.c endian.c objpool.c

if HAVE_LD_VERSION_SCRIPT
libx_la_LDFLAGS += -Wl,--version-script=$(srcdir)/libx.map
endif

if WINDOWS
libx_la_LDFLAGS += -lshlwapi -ldbghelp
else
libx_la_LDFLAGS += -ldl -lpthread
endif

if ENABLE_NETWORK
libx_la_SOURCES +=  event.c reactor.c socket.c asynet.c ipcsock.c
if WINDOWS
libx_la_LDFLAGS += -lws2_32
libx_la_SOURCES += wepoll.c mux_epoll.c
else
if HAVE_SYS_EPOLL_H
libx_la_SOURCES += mux_epoll.c 
else
if HAVE_POLL_H
libx_la_SOURCES += mux_poll.c 
endif
endif
endif
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
libx_la_SOURCES +=  aes.c sha256.c random.c md5.c
endif

if ENABLE_CHARMAP
libx_la_SOURCES +=  charmap.c charmap_big5.c  charmap_euckr.c  charmap_gbk.c  charmap_sjis.c
endif

EXTRA_DIST = strargv.inc wepoll.h libx.map
MAINTAINERCLEANFILES = Makefile.in
maintainer-clean-local:
	$(RM) -r .deps .cache

