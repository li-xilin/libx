#include "x/test.h"
#include "x/uidpool.h"
#include "x/dump.h"
#include <unistd.h>
#include <stdio.h>
#include <time.h>

static void fuzzing(ut_runner *r)
{
	int res;
	x_uidpool pool;
#define MAX 10
	x_uidpool_init(&pool, MAX);

	x_uidlink ul[MAX];
	for (int i = 0; i < MAX; i++) {
		res = x_uidpool_get(&pool, &ul[i]);
		ut_assert_uint_equal(r, 0, res);
		ut_assert_uint_equal(r, i + 1, ul[i].num);
	}
	x_uidlink tmp1;
	res = x_uidpool_get(&pool, &tmp1);
	ut_assert_int_equal(r, -1, res);

	x_uidlink tmp2;
	x_uidpool_put(&pool, &ul[5]);
	x_uidpool_get(&pool, &tmp2);
	ut_assert_uint_equal(r, 6, tmp2.num);

	x_uidlink tmp3;
	x_uidpool_put(&pool, &ul[3]);
	x_uidpool_get(&pool, &tmp3);
	ut_assert_uint_equal(r, 4, tmp3.num);

	x_uidlink tmp4;
	x_uidpool_put(&pool, &ul[1]);
	x_uidpool_get(&pool, &tmp4);
	ut_assert_uint_equal(r, 2, tmp4.num);

	x_uidlink tmp5;
	x_uidpool_put(&pool, &ul[9]);
	x_uidpool_get(&pool, &tmp5);
	ut_assert_uint_equal(r, 10, tmp5.num);
}

void uidpool_test_init(ut_suite *s)
{
	ut_suite_init(s, "uidpool.h");
	ut_suite_add(s, fuzzing);
}
