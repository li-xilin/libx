#include "x/thread.h"
#include "x/tss.h"
#include "x/string.h"
#include "x/flowctl.h"
#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <unistd.h>

static x_tss s_index_tss;

static void free_index_tss(void *p)
{
	printf("[-] clean tss: %d\n", (int)(uintptr_t)p);
}

static void thread_cleanup(void)
{
	size_t i = (size_t)x_thread_data();
	printf("[-] clean: %zd tid = %d\n", i, x_thread_native_id());
}

static int thread_entry(void)
{
	x_tss_set(&s_index_tss, x_thread_data());
	x_thread_sleep(100);

	size_t i = (size_t)x_tss_get(&s_index_tss);

	printf("[+] creat: %zd tid = %d\n", i, x_thread_native_id());
	for (int j = 0; j < 30; j++) {
		x_thread_sleep(100);
		x_thread_testcancel(1);
	}
	printf("thread %zd exit with function returned\n", i);
	return 0;
}

int main(int argc, char *argv[])
{
	size_t n = 10;
	x_thread *t[128] = { NULL };

	printf("=== create tss ===\n");
	x_tss_init(&s_index_tss, free_index_tss);

	printf("=== create %zd threads ===\n", n);
	x_repeat(n)
		t[_] = x_thread_create(thread_entry, thread_cleanup, (void *)_);

	printf("=== sleep 1000(ms) ===\n");
	x_thread_sleep(1000);

	printf("=== cancel %zd threads ===\n", n);
	x_thread_sleep(100);
	x_repeat(n)
		x_thread_cancel(t[_]);
	printf("=== sleep 1000(ms) ===\n");
	x_thread_sleep(1000);
	printf("=== join %zd threads ===\n", n);
	x_repeat(n) {
		x_thread_join(t[_], NULL);
		printf("thread %zd joined\n", _);
	}

	printf("=== create %zd threads ===\n", n);
	x_repeat(n)
		t[_] = x_thread_create(thread_entry, thread_cleanup, (void *)_);
	printf("=== detached %zd threads ===\n", n);
	x_repeat(n) {
		x_thread_free(t[_]);
		printf("thread %zd detached\n", _);
	}

	
	x_thread_sleep(5000);

	return 0;
}
