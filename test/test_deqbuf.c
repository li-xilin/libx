#include "x/test.h"
#include "x/deqbuf.h"
#include <unistd.h>
#include <stdio.h>
#include <time.h>
#include <x/string.h>

#if 0
static void dump(x_deqbuf *deq)
{
	x_ringbuf tmp = deq->map;
	int n = 0;
	while (x_ringbuf_used_size(&tmp)) {
		char *seg;
		x_ringbuf_pop(&tmp, &seg, sizeof seg);
		printf("%02d ", n++);
		for (int i = 0; i < deq->seg_size; i++) {
			printf("%c ", x_isdigit_7bit(seg[i]) ? seg[i] : '-');
		}
		putchar('\n');
	}
}
#endif

static void push_pop(ut_runner *r)
{
	char buf[10];
	size_t len;
	x_deqbuf deq = X_DEQBUF_INIT(deq);
	deq.seg_size = 3;

	x_deqbuf_push(&deq, "0123456", 7);
	ut_assert_uint_equal(r, 7, x_deqbuf_size(&deq));
	x_deqbuf_push(&deq, "0123456", 7);
	ut_assert_uint_equal(r, 14, x_deqbuf_size(&deq));
	x_deqbuf_push(&deq, "0123456", 7);
	ut_assert_uint_equal(r, 21, x_deqbuf_size(&deq));
	x_deqbuf_push(&deq, "0123456", 7);
	ut_assert_uint_equal(r, 28, x_deqbuf_size(&deq));

	len = x_deqbuf_pop(&deq, buf, sizeof buf - 1);
	ut_assert_mem_equal(r, "012345601", 9, buf, len);
	len = x_deqbuf_pop(&deq, buf, sizeof buf - 1);
	ut_assert_mem_equal(r, "234560123", 9, buf, len);
	len = x_deqbuf_pop(&deq, buf, sizeof buf - 1);
	ut_assert_mem_equal(r, "456012345", 9, buf, len);
	len = x_deqbuf_pop(&deq, buf, sizeof buf - 1);
	ut_assert_mem_equal(r, "6", 1, buf, len);
}

static void reverse_push_pop(ut_runner *r)
{
	char buf[10];
	size_t len;
	x_deqbuf deq = X_DEQBUF_INIT(deq);
	deq.seg_size = 3;

	x_deqbuf_push_front(&deq, "0123456", 7);
	ut_assert_uint_equal(r, 7, x_deqbuf_size(&deq));
	x_deqbuf_push_front(&deq, "0123456", 7);
	ut_assert_uint_equal(r, 14, x_deqbuf_size(&deq));
	x_deqbuf_push_front(&deq, "0123456", 7);
	ut_assert_uint_equal(r, 21, x_deqbuf_size(&deq));
	x_deqbuf_push_front(&deq, "0123456", 7);
	ut_assert_uint_equal(r, 28, x_deqbuf_size(&deq));

	len = x_deqbuf_pop_back(&deq, buf, sizeof buf - 1);
	ut_assert_mem_equal(r, "560123456", 9, buf, len);
	len = x_deqbuf_pop_back(&deq, buf, sizeof buf - 1);
	ut_assert_mem_equal(r, "345601234", 9, buf, len);
	len = x_deqbuf_pop_back(&deq, buf, sizeof buf - 1);
	ut_assert_mem_equal(r, "123456012", 9, buf, len);
	len = x_deqbuf_pop_back(&deq, buf, sizeof buf - 1);
	ut_assert_mem_equal(r, "0", 1, buf + 9 - len, len);
}

static void size_limit(ut_runner *r)
{
	char buf[10];
	size_t len;
	x_deqbuf deq = X_DEQBUF_INIT3(deq, 3, 1);

	x_deqbuf_push_front(&deq, "0123456", 7);
	len = x_deqbuf_pop_back(&deq, buf, sizeof buf - 1);
	ut_assert_mem_equal(r, "456", 3, buf + 9 - len, len);

	x_deqbuf_clear(&deq);

	x_deqbuf_push(&deq, "0123456", 7);
	len = x_deqbuf_pop(&deq, buf, sizeof buf - 1);
	ut_assert_mem_equal(r, "012", 3, buf, len);
}

static void peek_front(ut_runner *r)
{
	char buf[10];
	size_t len;
	x_deqbuf deq = X_DEQBUF_INIT3(deq, 3, 4);

	x_deqbuf_push(&deq, "90123456", 8);
	x_deqbuf_pop(&deq, NULL, 1);
	len = x_deqbuf_peek(&deq, 0, buf, 3);
	ut_assert_mem_equal(r, "012", 3, buf, len);
	len = x_deqbuf_peek(&deq, 1, buf, 3);
	ut_assert_mem_equal(r, "123", 3, buf, len);
	len = x_deqbuf_peek(&deq, 2, buf, 3);
	ut_assert_mem_equal(r, "234", 3, buf, len);
	len = x_deqbuf_peek(&deq, 3, buf, 3);
	ut_assert_mem_equal(r, "345", 3, buf, len);
	len = x_deqbuf_peek(&deq, 4, buf, 3);
	ut_assert_mem_equal(r, "456", 3, buf, len);
	len = x_deqbuf_peek(&deq, 5, buf, 3);
	ut_assert_mem_equal(r, "56", 2, buf, len);
	len = x_deqbuf_peek(&deq, 6, buf, 3);
	ut_assert_mem_equal(r, "6", 1, buf, len);
	len = x_deqbuf_peek(&deq, 7, buf, 3);
	ut_assert_mem_equal(r, "", 0, buf, len);
	len = x_deqbuf_peek(&deq, 8, buf, 3);
	ut_assert_mem_equal(r, "", 0, buf, len);
}

static void peek_backward(ut_runner *r)
{
	char buf[10];
	size_t len;
	x_deqbuf deq = X_DEQBUF_INIT3(deq, 3, 4);

	x_deqbuf_push(&deq, "90123456", 8);
	x_deqbuf_pop(&deq, NULL, 1);
	len = x_deqbuf_peek_backward(&deq, 0, buf, 3);
	ut_assert_mem_equal(r, "456", 3, buf, len);
	len = x_deqbuf_peek_backward(&deq, 1, buf, 3);
	ut_assert_mem_equal(r, "345", 3, buf, len);
	len = x_deqbuf_peek_backward(&deq, 2, buf, 3);
	ut_assert_mem_equal(r, "234", 3, buf, len);
	len = x_deqbuf_peek_backward(&deq, 3, buf, 3);
	ut_assert_mem_equal(r, "123", 3, buf, len);
	len = x_deqbuf_peek_backward(&deq, 4, buf, 3);
	ut_assert_mem_equal(r, "012", 3, buf, len);
	len = x_deqbuf_peek_backward(&deq, 5, buf, 3);
	ut_assert_mem_equal(r, "01", 2, buf + 1, len);
	len = x_deqbuf_peek_backward(&deq, 6, buf, 3);
	ut_assert_mem_equal(r, "0", 1, buf + 2, len);
	len = x_deqbuf_peek_backward(&deq, 7, buf, 3);
	ut_assert_mem_equal(r, "", 0, NULL, len);
	len = x_deqbuf_peek_backward(&deq, 8, buf, 3);
	ut_assert_mem_equal(r, "", 0, NULL, len);
}


void deqbuf_test_init(ut_suite *s)
{
	ut_suite_init(s, "deqbuf.h");
	ut_suite_add(s, push_pop);
	ut_suite_add(s, reverse_push_pop);
	ut_suite_add(s, size_limit);
	ut_suite_add(s, peek_front);
	ut_suite_add(s, peek_backward);
}
