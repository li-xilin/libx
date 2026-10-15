#include "x/invoke.h"
#include "x/tsignal.h"
#include "x/reactor.h"
#include "x/thread.h"
#include <stdio.h>
#include <stdlib.h>

static x_reactor r;

static void timeout(x_event *e)
{
	fprintf(stderr, "timer: tid = %ld\n", (unsigned long)x_thread_native_id());
}

static int invoke_handler_sync(void *data)
{
	fprintf(stderr, "invoke sync: data = %d, tid = %ld\n",
			(int)(uintptr_t)data, (unsigned long)x_thread_native_id());
	return 0;
}

static int invoke_handler_async(void *data)
{
	fprintf(stderr, "invoke asyn: data = %d, tid = %ld\n",
			(int)(uintptr_t)data, (unsigned long)x_thread_native_id());
	return 0;
}

int thread(void)
{
	fprintf(stderr, "invoke caller: tid = %ld\n", (unsigned long)x_thread_native_id());
	x_thread_sleep(500);
	while (1) {
		x_thread_sleep(1000);
		x_thread_testcancel(0);
		x_invoke_sync(&r, invoke_handler_sync, (void *)1);
		x_invoke_async(&r, invoke_handler_async, (void *)2);
	}
}

void signal_handler(int sig)
{
	if (sig == X_TSIGNAL_INT)
		x_reactor_break(&r);
}

int main(void)
{
	x_reactor_init(&r);
	if (x_tsignal_set(signal_handler)) {
		perror("x_tsignal_set");
		exit(1);
	}
	x_thread *thd = x_thread_create(thread, NULL, NULL);
	if (!thd) {
		perror("x_thread_create");
		exit(1);
	}
	x_evtimer timer = X_EVTIMER_INIT(1000, 0, timeout);
	x_reactor_add(&r, &timer.base);
	while (x_reactor_wait(&r) > 0) {
		x_reactor_do_events(&r);
	}
	x_thread_cancel(thd);
	x_thread_join(thd, NULL);
	x_reactor_free(&r);
	fprintf(stderr, "\nexiting\n");
}

