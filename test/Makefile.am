noinst_PROGRAMS = test
test_CFLAGS = $(regular_CFLAGS) -I$(top_srcdir)/include -D_POSIX_C_SOURCE=200112L -pthread
test_LDADD = $(top_builddir)/libx/libx.la
test_SOURCES = main.c test_future.c test_index.c test_pathset.c test_pipe.c test_hash.c \
	test_hfsm.c test_sklist.c test_uidpool.c test_ringbuf.c test_deqbuf.c test_hmap.c \
	test_objpool.c test_bitmap.c test_array.c

if ENABLE_REGEX
test_SOURCES += test_regex.c 
test_CFLAGS += -DTEST_REGEX
endif

if ENABLE_CRYPTO
test_SOURCES += test_aes.c test_sha256.c test_md5.c
test_CFLAGS += -DTEST_CRYPTO
endif

if ENABLE_CHARMAP
test_SOURCES += test_charmap.c
test_CFLAGS += -DTEST_CHARMAP
endif

MAINTAINERCLEANFILES = Makefile.in
maintainer-clean-local:
	$(RM) -r .deps .cache

