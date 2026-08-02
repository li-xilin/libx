#include "x/thread.h"
#include "x/thrpool.h"
#include "x/string.h"
#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <unistd.h>

#define NUM_THREADS 5
#define NUM_ITEMS 40
#define DELAY_MS 500

struct value
{
	x_thrwork work;
	int num;
};


static void work_handler(x_thrwork *work)
{
	struct value *val = x_container_of(work, struct value, work);
	int  old = val->num;

	val->num *= -1;
	int self = x_thread_native_id();
	printf("hash(thread)=%zx old=%d, val=%d\n",x_memhash(&self, sizeof self) , old, val->num);
	fflush(stdout);

	x_thread_sleep(DELAY_MS);
}

int main(int argc, char *argv[])
{
	x_thrpool pool;
	x_thrpool_init(&pool, NUM_THREADS);
	struct value vals[NUM_ITEMS];

	for (int i = 0; i < NUM_ITEMS; i++) {
		x_thrwork_init(&vals[i].work, work_handler);
		vals[i].num = i;
		x_thrpool_add_work(&pool, &vals[i].work);
	}
	x_thrpool_sync(&pool);
	for (int i = 0; i < NUM_ITEMS; i++) {
		printf("%d ", vals[i].num);
		fflush(stdout);
	}
	putchar('\n');

	x_thrpool_free(&pool);
	return 0;
}
