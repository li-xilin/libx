noinst_PROGRAMS = 01_flowctl 02_logging 03_base64 04_heap 05_bitmap 06_trick 07_splay \
	08_memory 09_loadini 10_rope 11_tpool 12_dump 13_thread 14_list 15_test 17_errno 18_uchar
if ENABLE_EDIT
noinst_PROGRAMS += 16_edit 
endif

if ENABLE_NETWORK
noinst_PROGRAMS += 19_reactor 
endif

if ENABLE_JSON
noinst_PROGRAMS += 20_json
endif

AM_CFLAGS = $(regular_CFLAGS) -I$(top_srcdir)/include -D_POSIX_C_SOURCE=200112L -pthread
LDADD = $(top_builddir)/libx/libx.la
#if WINDOWS
#AM_LDFLAGS = -no-fast-install
#endif


DISTCLEANFILES = Makefile.in
EXTRA_DIST = $(wildcard *.c) example.ini
dist-hook:
	$(RM) $(distdir)/Makefile.in

