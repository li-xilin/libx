noinst_PROGRAMS = 01_flowctl 02_logging 03_base64 04_heap 05_bitmap 06_trick 07_splay \
	08_memory 09_loadini 10_rope 11_thrpool 12_dump 13_thread 14_list 15_test 17_errno \
	18_uchar 22_fwalker 23_guard 24_backtrace 25_tsignal 27_invoke

if ENABLE_EDIT
noinst_PROGRAMS += 16_edit 
endif

if ENABLE_NETWORK
noinst_PROGRAMS += 19_reactor 26_ipc 28_async_echo 29_async_dns
endif

if ENABLE_JSON
noinst_PROGRAMS += 20_json
endif

if ENABLE_CRYPTO
noinst_PROGRAMS += 21_mt19937
endif

AM_CFLAGS = -I$(top_srcdir)/include
LDADD = $(top_builddir)/libx/libx.la

EXTRA_DIST = $(wildcard *.c) example.ini
MAINTAINERCLEANFILES = Makefile.in
maintainer-clean-local:
	$(RM) -r .deps .cache
