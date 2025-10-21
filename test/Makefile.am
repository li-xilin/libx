noinst_PROGRAMS = test

AM_CFLAGS = $(regular_CFLAGS) -I$(top_srcdir)/include -D_POSIX_C_SOURCE=200112L -pthread
test_LDADD = $(top_builddir)/libx/libx.la
test_SOURCES = test_all.c test_future.c test_index.c test_pathset.c

if ENABLE_REGEX
test_SOURCES += test_regex.c 
AM_CFLAGS += -DTEST_REGEX
endif

if ENABLE_CRYPTO
test_SOURCES += test_aes.c test_sha256.c
AM_CFLAGS += -DTEST_CRYPTO
endif

DISTCLEANFILES = Makefile.in
dist-hook:
	$(RM) $(distdir)/Makefile.in

