#include "x/test.h"
#include "x/strbuf.h"
#include "x/sklist.h"
#include "x/dump.h"
#include <unistd.h>
#include <stdio.h>
#include <time.h>

#define ARR { 3, 5, 2, 7, 1, 0, 4, 9, 6, 8, }
#define DUMP "sklist {0, 1, 2, 3, 4, 5, 6, 7, 8, 9}"

int compare(const void *a, const void *b)
{
	const int *x = a, *y = b;
	return *x - *y;
}

x_dump *dump(const void *p) {
	const int *x = p;
	return x_dump_int(*x);
}

static void fuzzing(ut_runner *r)
{
	x_sklist skl;
	x_sklist_init(&skl, compare);

	int arr[] = ARR;

	for (int i = 0; i < x_arrlen(arr); i++) {
		x_sklist_find_or_insert(&skl, &arr[i]);
		ut_assert(r, x_sklist_sanitize(&skl));
	}

	x_strbuf sb = X_STRBUF_INIT;
	x_dump_to_strbuf(x_sklist_dump(&skl, dump), x_dump_default_format(), &sb);
	ut_assert_str_equal(r, DUMP, x_strbuf_str(&sb));

	for (int i = 0; i < x_arrlen(arr); i++) {
		x_sklist_remove(&skl, &arr[i]);
		ut_assert(r, x_sklist_sanitize(&skl));
	}
	ut_assert(r, skl.level == 0);
}

static void foreach(ut_runner *r)
{
	x_sklist skl;
	x_sklist_init(&skl, compare);
	int arr[] = ARR;
	for (int i = 0; i < x_arrlen(arr); i++)
		x_sklist_find_or_insert(&skl, &arr[i]);

	int prev = -1;
	x_sklist_foreach(cur, &skl) {
		if (prev == -1) {
			prev = *(int *)cur->data;
			continue;
		}
		ut_assert(r, prev < *(int *)cur->data);
	}
}

void sklist_test_init(ut_suite *s)
{
	ut_suite_init(s, "sklist.h");
	ut_suite_add(s, fuzzing);
	ut_suite_add(s, foreach);
}
