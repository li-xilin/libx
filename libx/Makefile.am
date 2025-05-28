lib_LTLIBRARIES = libx.la

AM_CFLAGS = $(regular_CFLAGS) -fPIC -I$(top_srcdir)/include -Wno-format-nonliteral

if WINDOWS
AM_CFLAGS += -D_WIN32_WINNT=0x0600 -DWIN32_LEAN_AND_MEAN
else
AM_CFLAGS += -D_POSIX_C_SOURCE=199309L 
endif

libx_la_LDFLAGS = -Wl,--version-script=$(srcdir)/libx.map -version-info $(LT_VERSION_INFO)
libx_la_SOURCES = assert.c base64.c bitmap.c dump.c dumpfmt.c heap.c ini.c log.c memory.c pipe.c splay.c string.c tcolor.c rope.c btnode.c

