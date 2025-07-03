#include "x/thread.h"
#include "x/tpool.h"
#include "x/string.h"
#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <unistd.h>

static void worker(void *arg);

static void worker(void *arg)
{
	int *val = arg;
	int  old = *val;

	*val *= -1;
	int self = x_thread_native_id();
	printf("hash(thread)=%zx old=%d, val=%d\n",x_memhash(&self, sizeof self) , old, *val);
	fflush(stdout);

	x_thread_sleep(300);
}

int main(int argc, char *argv[])
{
	x_tpool tm;
	size_t   i;

	static const size_t num_threads = 5;
	static const size_t num_items   = 40;

	x_tpool_init(&tm, num_threads);
	int vals[num_items];

	for (i=0; i<num_items; i++) {
		vals[i] = i;
		x_tpool_work *w = x_tpool_work_create(worker, vals +i);
		x_tpool_add_work(&tm, w);
	}

	x_tpool_wait(&tm);

	for (i=0; i<num_items; i++) {
		printf("%d ", vals[i]);
		fflush(stdout);
	}
	putchar('\n');

	x_tpool_destroy(&tm);
	return 0;
}
