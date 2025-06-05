noinst_PROGRAMS = 01_flowctl 02_logging 03_base64 04_heap 05_bitmap 06_trick 07_splay 08_memory 09_loadini 10_rope 11_tpool 12_dump

AM_CFLAGS = -I$(top_srcdir)/include -D_POSIX_SOURCE=200112L -pthread
LDADD = $(top_builddir)/libx/libx.la

EXTRA_DIST = $(wildcard *.c)
