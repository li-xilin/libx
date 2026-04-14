#include "x/hash.h"
#include "x/test.h"
#include "x/printf.h"
#include <unistd.h>
#include <stdio.h>
#include <time.h>

const char *str = "Some rise by sin, and some by virtue fall.";

static void murmur2(ut_runner *r)
{
	ut_assert_uint_equal(r, 2418629218U, x_hash_murmur(str, strlen(str), 0));
	ut_assert_uint_equal(r, 4761444356487400828U, x_hash64_murmur(str, strlen(str), 0));
}

static void lookup3(ut_runner *r)
{
	ut_assert_uint_equal(r, 2948963046U, x_hash_lookup3(str, strlen(str), 0));
	ut_assert_uint_equal(r, 12665699843894295128U, x_hash64_lookup3(str, strlen(str), 0));
}

static void crc(ut_runner *r)
{
	ut_assert_uint_equal(r, 3027047833U, x_hash_crc(str, strlen(str), 0));
	ut_assert_uint_equal(r, 11331088808630045615U, x_hash64_crc(str, strlen(str), 0));
}

void hash_test_init(ut_suite *s)
{
	ut_suite_init(s, "hash.h");
	ut_suite_add(s, murmur2);
	ut_suite_add(s, lookup3);
	ut_suite_add(s, crc);
}

