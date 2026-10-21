#include "x/invoke.h"
#include "x/tsignal.h"
#include "x/reactor.h"
#include "x/thread.h"
#include <stdio.h>

static x_reactor r;

void timeout(x_event *e)
{
	fprintf(stderr, "timer: tid = %ld\n", x_thread_native_id());
}

int invoke_handler(void *param, void *data)
{
	fprintf(stderr, "invok: tid = %ld\n", x_thread_native_id());
	return 0;
}

int thread(void)
{
	fprintf(stderr, "invoke caller: tid = %ld\n", x_thread_native_id());
	x_thread_sleep(500);
	while (1) {
		x_thread_sleep(1000);
		x_invoke_sync(&r, invoke_handler, NULL, NULL);
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
	x_tsignal_set(signal_handler);

	x_thread *thd = x_thread_create(thread, NULL, NULL);
	(void)thd;

	x_evtimer timer = X_EVTIMER_INIT(1000, 0, timeout);
	x_reactor_add(&r, &timer.base);
	while (x_reactor_wait(&r) > 0) {
		x_reactor_do_events(&r);
	}
	x_reactor_free(&r);
	fprintf(stderr, "\nexiting\n");
}

