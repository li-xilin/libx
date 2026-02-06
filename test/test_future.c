#include "x/test.h"
#include "x/future.h"
#include "x/thread.h"
#include <unistd.h>
#include <stdio.h>
#include <time.h>

#define N 16

x_fupool fupool;

static int thread(void)
{
	uint16_t id = (intptr_t)x_thread_data();
	x_promise prom;
	int *p = x_promise_start(&prom, &fupool, id);
	if (p) {
		*p = *p + 1;
	}
	x_thread_sleep(rand() % 100);
	x_promise_commit(&prom, 0);
	return 0;
}

static void future_wait_all(ut_runner *r)
{
	int i, arr[N];
	x_future fut[N];
	x_future *fut_list[N];
	x_fupool_init(&fupool);
	for (i = 0; i < N; i++) {
		arr[i] = i;
		fut_list[i] = fut + i;
		uint16_t id = x_future_init(fut + i, &fupool, arr + i);
		x_thread_create(thread, NULL, (void *)(intptr_t)id);
	}

	int id = x_future_wait_all(fut_list, N, -1);
	if (id == N) {
		ut_printf(r, "invalid result: id == N");
		goto out;
	}

	for (i = 0; i < N; i++) {
		ut_assert_int_equal(r, i + 1, arr[i]);
		x_future_free(fut + i);
	}
out:
	x_fupool_free(&fupool);
}

static void future_wait_any(ut_runner *r)
{
	int i, arr[N], id;
	x_future fut[N];
	x_future *fut_list[N];
	x_fupool_init(&fupool);
	for (i = 0; i < N; i++) {
		arr[i] = i;
		fut_list[i] = fut + i;
		id = x_future_init(fut + i, &fupool, arr + i);
		x_thread_create(thread, NULL, (void *)(intptr_t)(uint16_t)id);
	}

	while ((id = x_future_wait_any(fut_list, N, -1)) != N) {
		x_future_free(fut + id);
	}

	for (i = 0; i < N; i++) {
		ut_assert_int_equal(r, i + 1, arr[i]);
		x_future_free(fut + i);
	}
	x_fupool_free(&fupool);
}

void future_test_init(ut_suite *s)
{
	srand(time(NULL));
	ut_suite_init(s, "future.h");
	ut_suite_add(s, future_wait_all);
	ut_suite_add(s, future_wait_any);
}
