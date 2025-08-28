#include "x/reactor.h"
#include "x/socket.h"
#include "x/thread.h"
#include "x/types.h"
#include <stdio.h>

#define LISTEN_PORT 9999

static x_reactor r;
static x_evsocket evsock1, evsock2;
static x_evtimer timer1, timer2;
static x_evobject evobj1;
static bool have_client = false;

static x_sock sock_listen(short port)
{
	x_sock sock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
	struct sockaddr_in si = {
		.sin_family = AF_INET,
		.sin_addr.s_addr = INADDR_ANY,
		.sin_port = htons(port),
	};
	if (bind(sock, (struct sockaddr *)&si, sizeof si) == -1) {
		fprintf(stderr, "bind error (%s)", strerror(errno));
		exit(1);
	}

	fprintf(stderr, "echo server started, listening on port %hd\n", port);

	listen(sock, 256);
	return sock;
}

static int another_thread(void)
{
	while (1) {
		x_thread_sleep(3500);
		evobj1.base.res_flags |= X_EV_READ;
		x_reactor_break(&r);
	}
	return 0;
}

static void on_timer1_timeout(x_evtimer *ev)
{
	fprintf(stderr, ".");
}

static void on_timer2_timeout(x_evtimer *ev)
{
	fputc('\n', stderr);
}

static void on_sock1_event(x_evsocket *ev)
{
	x_sock cli = accept(evsock1.sock, NULL, NULL);
	fprintf(stderr, "\nnew connection %d\n", (int)cli);
	if (have_client) {
		fprintf(stderr, "\nnconnection already exited, abort.\n");
		x_sock_close(cli);
	}
	else {
		x_evsocket_init(&evsock2, cli, X_EV_READ, NULL);
		x_reactor_add(&r, &evsock2.base);
		have_client = true;
	}
}

static void on_sock2_event(x_evsocket *ev)
{
	char buf[1024];
	int len = recv(evsock2.sock, buf, sizeof buf - 1, 0);
	if (len > 0) {
		buf[len] = '\0';
		fprintf(stderr, "\ndata: %s", buf);
		x_sock_sendall(evsock2.sock, buf, len);
	}
	else {
		if (len < 0)
			fprintf(stderr, "\nreceive data error\n");
		else
			fprintf(stderr, "\nsocket %d closed\n", (int)evsock2.sock);
		x_sock_close(evsock2.sock);
		have_client = false;
		x_reactor_remove(&r, &evsock2.base);
	}
}

int main(void)
{
	x_reactor_init(&r);

	x_evtimer_init(&timer1, 5, X_EV_ACCURATE, NULL);
	x_reactor_add(&r, &timer1.base);

	x_evtimer_init(&timer2, 500, X_EV_ACCURATE, NULL);
	x_reactor_add(&r, &timer2.base);

	x_evsocket_init(&evsock1, sock_listen(LISTEN_PORT), X_EV_READ, NULL);
	x_reactor_add(&r, &evsock1.base);

	x_evobject_init(&evobj1, X_EV_READ, NULL);
	x_reactor_add(&r, &evobj1.base);

	x_thread_create(another_thread, NULL, NULL);

	while (x_reactor_wait(&r)) {
		x_event *e = NULL;
		while ((e = x_reactor_pop_event(&r))) {
			if (e == &timer1.base)
				on_timer1_timeout(&timer1);
			else if (e == &timer2.base)
				on_timer2_timeout(&timer1);
			else if (e == &evsock1.base)
				on_sock1_event(&evsock1);
			else if (e == &evsock2.base)
				on_sock2_event(&evsock1);
			else if (e == &evobj1.base) {
				fprintf(stderr, "\nobject is readable!\n");
				e->res_flags = 0;
			}
		}
	}
}

