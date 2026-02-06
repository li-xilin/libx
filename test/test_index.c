#include "x/test.h"
#include "x/index.h"
#include <unistd.h>
#include <stdio.h>
#include <time.h>

struct element
{
	x_index index;
	int num;
};

static int compare(const x_index *idx1, const x_index *idx2)
{
	struct element *e1 = x_container_of(idx1, struct element, index);
	struct element *e2 = x_container_of(idx2, struct element, index);
	return e1->num - e2->num;
}

static void check_indexer(ut_runner *r, x_indexer *indexer, int n, int cnt)
{
	int real_cnt = 0;
	struct element pat;
	pat.num = n;
	const x_indexset *iset = x_indexer_find(indexer, &pat.index);
	if (iset) {
		x_indexset_foreach(i, iset) {
			struct element *elem = x_container_of(i, struct element, index);
			ut_assert_int_equal(r, n, elem->num);
			real_cnt++;
		}
	}
	ut_assert_int_equal(r, cnt, real_cnt);
}

static void basic(ut_runner *r)
{
	x_indexer indexer;
	x_indexer_init(&indexer, compare);

	int n[] = { 0, 1, 1, 2, 2, 2, 3, 3, 3, 3 };
	struct element arr[10];
	for (int i = 0; i < 10; i++) {
		arr[i].num = n[i];
		x_index_init(&arr[i].index);
	}

	for (int i = 0; i < 10; i++) {
		x_index_insert(&arr[i].index, &indexer);
	}

	check_indexer(r, &indexer, 0, 1);
	check_indexer(r, &indexer, 1, 2);
	check_indexer(r, &indexer, 2, 3);
	check_indexer(r, &indexer, 3, 4);

	x_index_remove(&arr[0].index);
	x_index_remove(&arr[1].index);
	x_index_remove(&arr[3].index);
	x_index_remove(&arr[7].index);

	check_indexer(r, &indexer, 0, 0);
	check_indexer(r, &indexer, 1, 1);
	check_indexer(r, &indexer, 2, 2);
	check_indexer(r, &indexer, 3, 3);

	x_indexer_free(&indexer);
}

void index_test_init(ut_suite *s)
{
	ut_suite_init(s, "index.h");
	ut_suite_add(s, basic);
}
