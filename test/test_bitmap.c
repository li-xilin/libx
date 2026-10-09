#include "x/test.h"
#include "x/bitmap.h"
#include <unistd.h>
#include <stdio.h>
#include <time.h>
#include <string.h>

static void bitmap_find(ut_runner *r)
{
	char buf[3] = { 0 };
	size_t pos;
	x_bitmap bm = X_BITMAP_INIT(buf, sizeof buf);
	size_t start = 4, len = x_bitmap_nbits(&bm) - 8;

	for (int i = start; i < len; i++) {
		x_bitmap_clear(&bm, 0);
		x_bitmap_set(&bm, i, 1);
		pos = x_bitmap_find(&bm, 1, start, len);
		ut_assert_int_equal(r, i, pos);
	}
	for (int i = start; i < len; i++) {
		x_bitmap_clear(&bm, 1);
		x_bitmap_set(&bm, i, 0);
		pos = x_bitmap_find(&bm, 0, start, len);
		ut_assert_int_equal(r, i, pos);
	}
}

static void bitmap_count(ut_runner *r)
{
	char buf[3] = { 0 };
	size_t sum;
	x_bitmap bm = X_BITMAP_INIT(buf, sizeof buf);
	size_t start = 4, len = x_bitmap_nbits(&bm) - 8;

	for (int i = start; i < len; i++) {
		x_bitmap_clear(&bm, 0);
		for (int j = start; j < i; j++)
			x_bitmap_set(&bm, j, 1);
		sum = x_bitmap_count(&bm, 1, start, len);
		ut_assert_int_equal(r, i - start, sum);
	}
	for (int i = start; i < len; i++) {
		x_bitmap_clear(&bm, 1);
		for (int j = start; j < i; j++)
			x_bitmap_set(&bm, j, 0);
		sum = x_bitmap_count(&bm, 0, start, len);
		ut_assert_int_equal(r, i - start, sum);
	}
}

static void bitmap_bitwise_and(ut_runner *r)
{
	char buf[1] = { 0 };
	x_bitmap bm = X_BITMAP_INIT(buf, sizeof buf);

	x_bitmap_clear(&bm, 0);
	x_bitmap_and(&bm, 1, 0);
	ut_assert_int_equal(r, 0, x_bitmap_get(&bm, 1));

	x_bitmap_clear(&bm, 0);
	x_bitmap_and(&bm, 1, 1);
	ut_assert_int_equal(r, 0, x_bitmap_get(&bm, 1));

	x_bitmap_clear(&bm, 1);
	x_bitmap_and(&bm, 1, 0);
	ut_assert_int_equal(r, 0, x_bitmap_get(&bm, 1));

	x_bitmap_clear(&bm, 1);
	x_bitmap_and(&bm, 1, 1);
	ut_assert_int_equal(r, 1, x_bitmap_get(&bm, 1));
}

static void bitmap_bitwise_or(ut_runner *r)
{
	char buf[1] = { 0 };
	x_bitmap bm = X_BITMAP_INIT(buf, sizeof buf);

	x_bitmap_clear(&bm, 0);
	x_bitmap_or(&bm, 1, 0);
	ut_assert_int_equal(r, 0, x_bitmap_get(&bm, 1));

	x_bitmap_clear(&bm, 0);
	x_bitmap_or(&bm, 1, 1);
	ut_assert_int_equal(r, 1, x_bitmap_get(&bm, 1));

	x_bitmap_clear(&bm, 1);
	x_bitmap_or(&bm, 1, 0);
	ut_assert_int_equal(r, 1, x_bitmap_get(&bm, 1));

	x_bitmap_clear(&bm, 1);
	x_bitmap_or(&bm, 1, 1);
	ut_assert_int_equal(r, 1, x_bitmap_get(&bm, 1));
}

static void bitmap_bitwise_xor(ut_runner *r)
{
	char buf[1] = { 0 };
	x_bitmap bm = X_BITMAP_INIT(buf, sizeof buf);

	x_bitmap_clear(&bm, 0);
	x_bitmap_xor(&bm, 1, 0);
	ut_assert_int_equal(r, 0, x_bitmap_get(&bm, 1));

	x_bitmap_clear(&bm, 0);
	x_bitmap_xor(&bm, 1, 1);
	ut_assert_int_equal(r, 1, x_bitmap_get(&bm, 1));

	x_bitmap_clear(&bm, 1);
	x_bitmap_xor(&bm, 1, 0);
	ut_assert_int_equal(r, 1, x_bitmap_get(&bm, 1));

	x_bitmap_clear(&bm, 1);
	x_bitmap_xor(&bm, 1, 1);
	ut_assert_int_equal(r, 0, x_bitmap_get(&bm, 1));
}

static void bitmap_bitwise_not(ut_runner *r)
{
	char buf[1] = { 0 };
	x_bitmap bm = X_BITMAP_INIT(buf, sizeof buf);

	x_bitmap_clear(&bm, 0);
	x_bitmap_not(&bm, 1);
	ut_assert_int_equal(r, 1, x_bitmap_get(&bm, 1));

	x_bitmap_clear(&bm, 1);
	x_bitmap_not(&bm, 1);
	ut_assert_int_equal(r, 0, x_bitmap_get(&bm, 1));
}

void bitmap_test_init(ut_suite *s)
{
	ut_suite_init(s, "bitmap.h");
	ut_suite_add(s, bitmap_find);
	ut_suite_add(s, bitmap_count);
	ut_suite_add(s, bitmap_bitwise_and);
	ut_suite_add(s, bitmap_bitwise_or);
	ut_suite_add(s, bitmap_bitwise_xor);
	ut_suite_add(s, bitmap_bitwise_not);
}

