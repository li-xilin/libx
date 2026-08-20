#include <x/tsignal.h>
#include <x/thread.h>
#include <x/memory.h>
#include <stdio.h>
#include <string.h>
static bool s_exit = false;
void signal_handler(int sig)
{
	switch (sig) {
		case X_TSIGNAL_INT:
			fprintf(stderr, "Signal INT\n");
			s_exit = true;
			break;
		case X_TSIGNAL_QUIT:
			fprintf(stderr, "Signal QUIT\n");
			s_exit = true;
			break;
		case X_TSIGNAL_CLOSE:
			fprintf(stderr, "Signal CLOSE\n");
			s_exit = true;
			break;
		case X_TSIGNAL_NOMEM:
			fprintf(stderr, "Signal NOMEM\n");
			break;
	}
}

int main(void)
{
	x_tsignal_set(signal_handler);
	while (!s_exit)
		x_thread_sleep(200);
	return 0;
}
