#include "x/reactor.h"
#include "x/socket.h"
#include "x/thread.h"
#include <stdio.h>

static x_reactor r;
static x_evsocket evsock1, evsock2;
static x_evtimer timer1, timer2;
static x_evobject evobj1;

x_sock sock_listen(short port)
{
	x_sock sock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
	struct sockaddr_in si = {
		.sin_family = AF_INET,
		.sin_addr.s_addr = INADDR_ANY,
		.sin_port = htons(port),
	};
	bind(sock, (struct sockaddr *)&si, sizeof si);
	listen(sock, 256);
	return sock;
}

int another_thread(void)
{
	while (1) {
		x_thread_sleep(3500);
		evobj1.base.res_flags |= X_EV_READ;
		x_reactor_break(&r);
	}
	return 0;
}

int main(void)
{
	bool have_client = false;
	x_reactor_init(&r);
	x_evtimer_init(&timer1, 5, X_EV_ACCURATE, NULL);
	x_evtimer_init(&timer2, 500, X_EV_ACCURATE, NULL);
	x_evsocket_init(&evsock1, sock_listen(9999), X_EV_READ, NULL);
	x_evobject_init(&evobj1, X_EV_READ, NULL);
	x_reactor_add(&r, &timer1.base);
	x_reactor_add(&r, &timer2.base);
	x_reactor_add(&r, &evsock1.base);
	x_reactor_add(&r, &evobj1.base);

	x_thread_create(another_thread, NULL, NULL);

	while (x_reactor_wait(&r)) {
		x_event *e = NULL;
		while ((e = x_reactor_pop_event(&r))) {
			if (e == &timer1.base) {
				fprintf(stderr, ".");
			}
			else if (e == &timer2.base) {
				fputc('\n', stderr);
			}
			else if (e == &evsock1.base) {
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
			else if (e == &evsock2.base) {
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
			else if (e == &evobj1.base) {
				fprintf(stderr, "\nobject is readable!\n");
				e->res_flags = 0;
			}
		}
	}
}
