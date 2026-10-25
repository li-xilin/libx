#include "x/asynet.h"
#include "x/reactor.h"
#include "x/socket.h"
#include "x/ringbuf.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define BUF_SIZE 8
#define SEND_TIMEOUT 5000
#define RECV_TIMEOUT 5000

struct client {
	x_link link;
	x_stream *stm;
	x_ringbuf rb;
	char buf[BUF_SIZE + 1];
	bool sending;
	bool recving;
};

static void on_accepted(x_sock fd, void *user);
void on_data_sent(int err, size_t transfered, void *user);
void on_data_received(int err, size_t transfered, void *user);

static x_reactor r;
x_list client_list = X_LIST_INIT(client_list);


void on_data_sent(int err, size_t transfered, void *user)
{
	struct client *c = user;
	if (err > 0) {
		fprintf(stderr, "on_data_sent error %s\n", strerror(err));
		return;
	}
	x_ringbuf_commit_read(&c->rb, transfered);
	if (x_ringbuf_is_empty(&c->rb)) {
		c->sending = false;
	}
	else {
		x_iovec iovec = x_ringbuf_acquire_read(&c->rb);
		x_stream_send_buf(c->stm, iovec.base, iovec.len, on_data_sent, c);
	}
	if (!c->recving) {
		x_iovec iovec = x_ringbuf_acquire_write(&c->rb);
		x_stream_recv_buf(c->stm, iovec.base, iovec.len, on_data_received, c);
		c->recving = true;
	}
}

void on_data_received(int err, size_t transfered, void *user)
{
	struct client *c = user;
	if (err > 0 || transfered == 0) {
		if (err > 0)
			fprintf(stderr, "on_data_received error %s\n", strerror(err));
		else
			fprintf(stderr, "EOF received\n");
		return;
	}

	x_iovec iovec = x_ringbuf_acquire_write(&c->rb);
	printf("%.*s", (int)transfered, (char *)iovec.base);

	x_ringbuf_commit_write(&c->rb, transfered);
	if (x_ringbuf_is_full(&c->rb)) {
		c->recving = false;
	}
	else {
		x_iovec iovec = x_ringbuf_acquire_write(&c->rb);
		x_stream_recv_buf(c->stm, iovec.base, iovec.len, on_data_received, c);
	}
	if (!c->sending) {
		x_iovec iovec = x_ringbuf_acquire_read(&c->rb);
		x_stream_send_buf(c->stm, iovec.base, iovec.len, on_data_sent, c);
		c->sending = true;
	}
}

static void on_stream_finished(x_stream *stream, void *user)
{
	struct client *c = user;
	fprintf(stderr, "client %d closed\n", x_stream_sock(c->stm));
	x_stream_free(c->stm);
	x_list_del(&c->link);
	x_free(c);
}

static void on_accepted(x_sock fd, void *user)
{
	x_reactor *r = user;
	struct client *c = x_malloc(NULL, sizeof *c);
	c->stm = x_stream_create(r, fd, SEND_TIMEOUT, RECV_TIMEOUT, on_stream_finished, c);
	c->sending = false, c->recving = true;
	x_ringbuf_init(&c->rb, c->buf, sizeof c->buf);
	x_list_add_back(&client_list, &c->link);
	x_iovec iovec = x_ringbuf_acquire_write(&c->rb);
	x_stream_recv_buf(c->stm, iovec.base, iovec.len, on_data_received, c);
	fprintf(stderr, "client %d connected\n", fd);
}

int main(void)
{
	if (x_reactor_init(&r)) {
		perror("x_reactor_init");
		exit(1);
	}
	x_hostport hp = X_HOSTPORT_INIT("127.0.0.1", 9001);
	x_listener *lnr = x_listener_create(&r, &hp, on_accepted, &r);
	if (!lnr) {
		perror("x_listener_create");
		exit(1);
	}
	if (x_listener_start(lnr)) {
		perror("x_listener_start");
		exit(1);
	}
	fprintf(stderr, "echo server is listening at 127.0.0.1:9001\n");

	while (x_reactor_wait(&r) > 0)
		x_reactor_do_events(&r);

	x_listener_free(lnr);
	x_reactor_free(&r);
}

