#include <x/debug.h>
#include <x/strbuf.h>
#include <x/file.h>
#include <stdio.h>
#include <signal.h>

static void signal_handler(int sig)
{
	x_trace t;
	x_strbuf sb;
	x_strbuf_init(&sb);
	x_trace_back(&t);
	x_trace_dump(&t, &sb);
	x_fprintf(stderr, x_u("%S\n"), x_strbuf_data(&sb));
	x_strbuf_free(&sb);
}

void bar(void)
{
	int *p = NULL;
	*p = 1;
}

void foo(void)
{
	bar();
}

int main(void)
{
	x_setmode_utf8(stderr);
	signal(SIGABRT, signal_handler);
	signal(SIGSEGV, signal_handler);
	foo();
}

