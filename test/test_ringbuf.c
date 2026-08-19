#include "x/flowctl.h"
#include "x/test.h"
#include "x/ringbuf.h"
#include <unistd.h>
#include <stdio.h>
#include <time.h>
#include <string.h>

#define assert_equal(ringbuf, data, size) \
{ \
	char tmp[MAXLEN]; \
	size_t l = x_ringbuf_read(ringbuf, tmp, MAXLEN); \
	ut_assert_mem_equal(r, data, size, tmp, l); \
}

#define MAXLEN 7 // limits to 27
#define ALPHABET "abcdefghijklmnopqrstuvwxyz"

static void make_ringbuf(x_ringbuf *rb, char *buf, size_t size, size_t offset, size_t len)
{
	assert(len < size);
	memset(buf, 0, size);
	for (int i = 0; i < len; i++)
		buf[(i + offset) % size] = 'a' + (i % 26);
	rb->buf = (void *)buf;
	rb->size = size;
	rb->front = offset;
	rb->rear = (offset + len) % size;
}

static void resize(ut_runner *r)
{
	char buf[MAXLEN];
	x_ringbuf rb;
	for (int len = 0; len < MAXLEN; len++) {
		for (int off = 0; off < MAXLEN; off++) {
			make_ringbuf(&rb, buf, MAXLEN, off, len); 
			x_ringbuf_resize(&rb, len + 1);
			assert_equal(&rb, ALPHABET, len);
		}
	}
}

static void pre_test(ut_runner *r)
{
	char buf[MAXLEN];
	x_ringbuf rb;
	for (int len = 0; len < MAXLEN; len++) {
		for (int off = 0; off < MAXLEN; off++) {
			make_ringbuf(&rb, buf, MAXLEN, off, len); 
			assert_equal(&rb, ALPHABET, len);
		}
	}
}

static void read_front(ut_runner *r)
{
	char buf[MAXLEN];
	x_ringbuf rb;
	for (int len = 0; len < MAXLEN; len++) {
		for (int off = 0; off < MAXLEN; off++) {
			char ac[2];
			make_ringbuf(&rb, buf, MAXLEN, off, len); 
			int n = x_ringbuf_read(&rb, ac, 2);
			ut_assert_mem_equal(r, ALPHABET, x_min(len, 2), ac, n);
			assert_equal(&rb, (char *)ALPHABET + 2, len - x_min(len, 2));
		}
	}
}

static void read_back(ut_runner *r)
{
	char buf[MAXLEN];
	x_ringbuf rb;
	for (int len = 0; len < MAXLEN; len++) {
		for (int off = 0; off < MAXLEN; off++) {
			char ac[2];
			make_ringbuf(&rb, buf, MAXLEN, off, len); 
			int n = x_ringbuf_read_back(&rb, ac, 2);
			ut_assert_mem_equal(r, (char *)ALPHABET + len - x_min(2, len),  x_min(2, len), ac, n);
			assert_equal(&rb, ALPHABET, len - x_min(2, len));
		}
	}
}

static void write_front(ut_runner *r)
{
	char buf[MAXLEN];
	x_ringbuf rb;
	for (int len = 0; len < MAXLEN; len++) {
		for (int off = 0; off < MAXLEN; off++) {
			make_ringbuf(&rb, buf, MAXLEN, off, len); 
			x_ringbuf_write_front(&rb, "AB", 2);
			assert_equal(&rb, (char *)"AB" ALPHABET + 2 - (x_min(len + 2, MAXLEN - 2 + 1) - len), x_min(len + 2, MAXLEN - 2 + 1));
		}
	}
}

static void write_back(ut_runner *r)
{
	char buf[MAXLEN];
	x_ringbuf rb;
	for (int len = 0; len < MAXLEN; len++) {
		for (int off = 0; off < MAXLEN; off++) {
			make_ringbuf(&rb, buf, MAXLEN, off, len); 
			char ex[32] = ALPHABET;
			x_ringbuf_write(&rb, "AB", 2);
			memcpy(ex + len, "AB", 2);
			assert_equal(&rb, ex, x_min(len + 2, MAXLEN - 1));
		}
	}
}

static void pour(ut_runner *r)
{
	char buf[MAXLEN];
	x_ringbuf rb;
	for (int len = 0; len < MAXLEN; len++) {
		for (int off = 0; off < MAXLEN; off++) {
			make_ringbuf(&rb, buf, MAXLEN, off, len); 
			char dest_buf[MAXLEN];
			x_ringbuf dest = X_RINGBUF_INIT(dest_buf, sizeof dest_buf);
			x_ringbuf_pour(&rb, &dest, 2);
			assert_equal(&rb, (char *)ALPHABET + 2, x_max(2, len) - 2);
			assert_equal(&dest, ALPHABET, x_min(2, len));
		}
	}
}

static void pour_back(ut_runner *r)
{
	char buf[MAXLEN];
	x_ringbuf rb;
	for (int len = 0; len < MAXLEN; len++) {
		for (int off = 0; off < MAXLEN; off++) {
			make_ringbuf(&rb, buf, MAXLEN, off, len); 
			char dest_buf[MAXLEN];
			x_ringbuf dest = X_RINGBUF_INIT(dest_buf, sizeof dest_buf);
			x_ringbuf_pour_backward(&rb, &dest, 2);
			assert_equal(&rb, ALPHABET, x_max(2, len) - 2);
			assert_equal(&dest, (char *)ALPHABET + len - x_min(2, len), x_min(2, len));
		}
	}
}

void ringbuf_test_init(ut_suite *s)
{
	ut_suite_init(s, "ringbuf.h");
	ut_suite_add(s, pre_test);
	ut_suite_add(s, read_front);
	ut_suite_add(s, read_back);
	ut_suite_add(s, write_front);
	ut_suite_add(s, write_back);
	ut_suite_add(s, resize);
	ut_suite_add(s, pour);
	ut_suite_add(s, pour_back);
}

