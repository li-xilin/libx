#include "x/test.h"
#include "x/objpool.h"
#include "x/dump.h"
#include <unistd.h>
#include <stdio.h>
#include <time.h>

static void objpool_fuzzing(ut_runner *r)
{
	int *arr[4096] = { 0 };
	x_objpool pool = X_OBJPOOL_INIT(35);
	for (int i = 0; i < x_arrlen(arr); i++) {
		arr[i] = x_objpool_alloc(&pool);
		ut_assert(r, arr[i]);
		ut_assert(r, x_objpool_sanitize(&pool));
		*arr[i] = i;
	}
	for (int i = 0; i < x_arrlen(arr); i++) {
		ut_assert_int_equal(r, i, *arr[i]);
		x_objpool_dealloc(&pool, arr[i]);
		ut_assert(r, x_objpool_sanitize(&pool));
	}
	ut_assert(r, x_list_is_empty(&pool.empty));
	ut_assert(r, x_list_is_empty(&pool.partial));
	ut_assert(r, x_list_is_empty(&pool.full));
}

void objpool_test_init(ut_suite *s)
{
	ut_suite_init(s, "objpool.h");
	ut_suite_add(s, objpool_fuzzing);
}

