#include "x/array.h"
#include "x/test.h"
#include "x/printf.h"
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <stdio.h>


static void array_push_pop(ut_runner *r)
{
	int N = 130;
	x_array a = X_ARRAY_INIT(sizeof(int));
	for (int i = 0; i < N; i++) {
		int *v = x_array_push_back(&a, &i);
		ut_assert_int_equal(r, i, *v);
		ut_assert_int_equal(r, i + 1, x_array_size(&a));
	}

	for (int i = N - 1; i >= 0; i--) {
		int *v = x_array_pop_back(&a);
		ut_assert_int_equal(r, i, *v);
		ut_assert_int_equal(r, i, x_array_size(&a));
		x_array_shrink(&a);
	}
}

static void array_foreach(ut_runner *r)
{
	int N = 32;
	x_array a = X_ARRAY_INIT(sizeof(int));
	for (int i = 0; i < N; i++)
		x_array_push_back(&a, &i);

	int n = 0;
	x_array_foreach(int, v, &a) {
		ut_assert_int_equal(r, n, *v);
		n++;
	}
}

static void array_insert(ut_runner *r)
{
	x_array a = X_ARRAY_INIT(sizeof(int));
	int v = -1;

	x_array_insert(&a, 0, &v);

	for (int i = 0; i < 16; i++)
		x_array_push_back(&a, &i);
	x_array_insert(&a, 8, &v);
	x_array_insert(&a, 18, &v);

	ut_assert_int_equal(r, 19, x_array_size(&a));
	ut_assert_int_equal(r, -1, X_ARRAY_AT(int, &a, 0));
	ut_assert_int_equal(r, -1, X_ARRAY_AT(int, &a, 8));
	ut_assert_int_equal(r, -1, X_ARRAY_AT(int, &a, 18));
}

static void array_remove(ut_runner *r)
{
	x_array a = X_ARRAY_INIT(sizeof(int));
	for (int i = 0; i < 16; i++)
		x_array_push_back(&a, &i);
	x_array_remove_at(&a, 15);
	x_array_remove_at(&a, 8);
	x_array_remove_at(&a, 0);

	x_array_foreach(int, v, &a) {
		ut_assert(r, *v != 15);
		ut_assert(r, *v != 8);
		ut_assert(r, *v != 0);
	}
}

void array_test_init(ut_suite *s)
{
	ut_suite_init(s, "array.h");
	ut_suite_add(s, array_push_pop);
	ut_suite_add(s, array_foreach);
	ut_suite_add(s, array_insert);
	ut_suite_add(s, array_remove);
}

