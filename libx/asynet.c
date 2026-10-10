/*
 * Copyright (c) 2026 Li Xilin <lixilin@gmx.com>
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */
#include "x/asynet.h"
#include "x/socket.h"
#include "x/event.h"
#include "x/reactor.h"
#include "x/log.h"
#include "x/memory.h"
#include "x/pipe.h"
#include "x/errno.h"
#include "x/sys.h"
#include <stdlib.h>
#include <stdio.h>

#define PENDING_LIMIT 256
#define CONNECT_TIMEOUT (1000 * 5)
#define TRANSFER_TIMEOUT (1000 * 10)

struct x_dialer_st {
	x_reactor *reactor;
	x_mset mset;
	x_list list;
};

struct x_listener_st {
	x_reactor *reactor;
	x_mset mset;
	x_evsocket listen_event;
	x_ipaddr ipaddr;
	short port;
	x_list result_list;
	x_listener_handler *on_accept;
	void *user;
};

struct x_stream_st {
	x_reactor *reactor;
	x_mset mset;
	x_evsocket sock_event;
	x_list send_list;
	x_list recv_list;
	x_evtimer error_event;
	int timeout_millisec;
	bool finished;
	int send_error, recv_error;
};

struct x_resolver_st {
	x_reactor *reactor;
	x_mset mset;
	x_thrpool thrpool;
	x_evobject complete_event;
	x_mutex lock;
	x_list query_list;
	x_list result_list;
};

struct x_timer_st {
	x_reactor *reactor;
	x_mset mset;
	x_uidpool uidpool;
};

struct pending
{
	x_link link;
	x_ipaddr ipaddr;
	uint16_t port;
	int error;
	x_evtimer timer;
	x_dialer *dialer;
	x_evsocket event;
	x_dialer_handler *on_connect;
	void *user;
};

struct transfer
{
	x_link link;
	size_t total_len;
	size_t transferred;
	size_t iovec_shift;
	uint16_t iovec_count, iovec_index;
	bool wait_all;
	int error;
	x_stream_handler *on_transferred;
	void *user;
	x_iovec iovec[];
};

struct query
{
	x_link link;
	x_resolver *rvr;
	x_thrwork work;
	struct addrinfo *res;
	x_resolver_handler *on_resolve;
	void *user;
	int error;
	char name[];
};

struct clock
{
	x_timer *timer;
	x_uidlink uid;
	x_evtimer timer_event;
	x_timer_handler *on_timedout;
	uint32_t interval;
	void *user;
};

static x_sock sock_connect(x_ipaddr *ipaddr, short port)
{
	x_sock fd = socket(ipaddr->family, SOCK_STREAM, IPPROTO_TCP);
	if (fd == X_BADSOCK) {
		x_eval_errno();
		goto fail;
	}
	if (x_sock_set_nonblocking(fd, true))
		goto fail;
	if (ipaddr->family == AF_INET) {
		struct sockaddr_in sa = {
			.sin_family = AF_INET,
			.sin_addr = ipaddr->un.in4,
			.sin_port = htons(port),
		};
		if (connect(fd, (void *)&sa, sizeof sa)) {
			if (errno != X_EINPROGRESS)
				goto fail;
		}
	}
	else if (ipaddr->family == AF_INET6) {
		struct sockaddr_in6 sa = {
			.sin6_family = AF_INET6,
			.sin6_addr = ipaddr->un.in6,
			.sin6_port = htons(port),
		};
		if (connect(fd, (void *)&sa, sizeof sa)) {
			if (errno != X_EINPROGRESS)
				goto fail;
		}
	}
	else
		abort();
	return fd;
fail:
	if (fd != X_BADSOCK)
		x_sock_close(fd);
	return X_BADSOCK;
}

x_dialer *x_dialer_create(x_reactor *reactor)
{
	x_dialer *dialer = x_malloc(NULL, sizeof *dialer);
	x_mset_init(&dialer->mset);
	dialer->reactor = reactor;
	x_list_init(&dialer->list);
	return dialer;
}

static void on_connect_event(x_event *ev)
{
	int error = 0;
	x_sock fd;
	struct pending *p = x_container_of(ev, struct pending, event.base);
	socklen_t len = sizeof(error);
	if (getsockopt(p->event.sock, SOL_SOCKET, SO_ERROR, (char *)&error, &len)) {
		error = errno;
		p->on_connect(error, X_BADSOCK, p->user);
	}
	else if (error) {
		x_sock_send_reset(p->event.sock);
		p->on_connect(error, X_BADSOCK, p->user);
	}
	else {
		fd = p->event.sock;;
		p->on_connect(0, fd, p->user);
	}
	x_reactor_remove(&p->event.base);
	x_reactor_remove(&p->timer.base);
	x_list_del(&p->link);
	x_free(p);
}

static void on_connect_error(x_event *ev)
{
	struct pending *p = x_container_of(ev, struct pending, timer.base);
	x_sock_close(p->event.sock);
	if (!p->error) {
		p->error = X_ETIMEDOUT;
		x_reactor_remove(&p->event.base);
	}
	p->on_connect(p->error, X_BADSOCK, p->user);
	x_sock_close(p->event.sock);
	x_list_del(&p->link);
	x_free(p);
}

void x_dialer_connect(x_dialer *dialer, const x_hostport *addr, x_dialer_handler *on_connect, void *user)
{
	x_sock fd = X_BADSOCK;
	int error;
	x_ipaddr ipaddr;
	if (x_parse_ipaddr(addr->host, &ipaddr)
			|| (fd = sock_connect(&ipaddr, addr->port)) == X_BADSOCK)
		error = errno;
	struct pending *s = x_malloc(&dialer->mset, sizeof *s);
	s->on_connect = on_connect;
	s->user = user;
	if (fd == X_BADSOCK) {
		s->error = error;
		x_evtimer_init(&s->timer, 0, X_EV_ONCE, on_connect_error);
		x_reactor_add(dialer->reactor, &s->timer.base);
	}
	else {
		s->error = 0;
		x_evsocket_init(&s->event, fd, X_EV_WRITE, on_connect_event);
		x_evtimer_init(&s->timer, CONNECT_TIMEOUT, X_EV_ONCE, on_connect_error);
		x_reactor_add(dialer->reactor, &s->event.base);
		x_reactor_add(dialer->reactor, &s->timer.base);
	}
	x_list_add_back(&dialer->list, &s->link);
}

void x_dialer_cancel(x_dialer *dialer)
{
	x_list_popeach(cur, &dialer->list) {
		struct pending *p = x_container_of(cur, struct pending, link);
		if (!p->error) {
			x_sock_close(p->event.sock);
			x_reactor_remove(&p->event.base);
			p->on_connect(X_ECANCELED, X_BADSOCK, p->user);
		}
		else {
			p->on_connect(p->error, X_BADSOCK, p->user);
		}
		x_reactor_remove(&p->timer.base);
		x_free(p);
	}
}

void x_dialer_free(x_dialer *dialer)
{
	if (!dialer)
		return;
	x_dialer_cancel(dialer);
	x_mset_free(&dialer->mset);
	x_free(dialer);
}

static void on_listen_event(x_event *ev)
{
	x_listener *lnr = x_container_of(ev, x_listener, listen_event.base);
	x_sock fd = accept(lnr->listen_event.sock, NULL, NULL);
	if (fd == X_BADSOCK) {
		x_perror("failed to accept client (%s)", strerror(errno));
		return;
	}
	lnr->on_accept(fd, lnr->user);
}

static x_sock sock_listen(x_ipaddr *ipaddr, short port)
{
	x_sock listen_fd = socket(ipaddr->family, SOCK_STREAM, IPPROTO_TCP);
	if (listen_fd == X_BADSOCK)
		return X_BADSOCK;
	int optval = 1;
	setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, (char *)&optval, sizeof(optval));

	if (ipaddr->family == AF_INET) {
		struct sockaddr_in sa = {
			.sin_family = AF_INET,
			.sin_addr = ipaddr->un.in4,
			.sin_port = htons(port),
		};
		if (bind(listen_fd, (void *)&sa, sizeof sa)) {
			return X_BADSOCK;
		}
	}
	else if (ipaddr->family == AF_INET6) {
		struct sockaddr_in6 sa = {
			.sin6_family = AF_INET6,
			.sin6_addr = ipaddr->un.in6,
			.sin6_port = htons(port),
		};
		if (bind(listen_fd, (void *)&sa, sizeof sa)) {
			x_eval_errno();
			return X_BADSOCK;
		}
	}
	else
		abort();
	if (listen(listen_fd, 256))
		return X_BADSOCK;
	return listen_fd;
}

x_listener *x_listener_create(x_reactor *reactor, const x_hostport *addr, x_listener_handler *on_accepted, void *user)
{
	x_ipaddr ipaddr;
	if (x_parse_ipaddr(addr->host, &ipaddr))
		return NULL;
	x_listener *lnr = x_malloc(NULL, sizeof *lnr);
	lnr->ipaddr = ipaddr;
	lnr->port = addr->port;
	lnr->reactor = reactor;
	x_mset_init(&lnr->mset);
	x_evsocket_init(&lnr->listen_event, -1, 0, NULL);
	x_list_init(&lnr->result_list);
	lnr->on_accept = on_accepted;
	lnr->user = user;
	return lnr;
}

short x_listener_port(x_listener *lnr)
{
	struct sockaddr_in addr = { 0 };
	socklen_t len = sizeof(addr);
	getsockname(lnr->listen_event.sock, (struct sockaddr *)&addr, &len);
	return ntohs(addr.sin_port);
}

int x_listener_start(x_listener *lnr)
{
	if (lnr->listen_event.base.ev_flags & X_EV_REACTING)
		return 0;
	x_sock fd = sock_listen(&lnr->ipaddr, lnr->port);
	if (fd == X_BADSOCK)
		return -1;
	x_evsocket_init(&lnr->listen_event, fd, X_EV_READ, on_listen_event);
	x_reactor_add(lnr->reactor, &lnr->listen_event.base);
	return 0;
}

void x_listener_stop(x_listener *lnr)
{
	if (!(lnr->listen_event.base.ev_flags & X_EV_REACTING))
		return;
	x_sock_close(lnr->listen_event.sock);
	x_reactor_remove(&lnr->listen_event.base);
}

void x_listener_free(x_listener *lnr)
{
	if (!lnr)
		return;
	x_listener_stop(lnr);
	x_mset_free(&lnr->mset);
	x_free(lnr);
}

static void flush_all_send(x_stream *stream, int error)
{
	x_list_popeach(cur, &stream->send_list) {
		struct transfer *p = x_container_of(cur, struct transfer, link);
		p->on_transferred(error, p->transferred, p->user);
		x_free(p);
	}
	stream->sock_event.base.ev_flags &= ~X_EV_WRITE;
	x_reactor_modify(&stream->sock_event.base);
}

static void flush_all_recv(x_stream *stream, int error)
{
	x_list_popeach(cur, &stream->recv_list) {
		struct transfer *p = x_container_of(cur, struct transfer, link);
		p->on_transferred(error, p->transferred, p->user);
		x_free(p);
	}
	stream->sock_event.base.ev_flags &= ~X_EV_READ;
	x_reactor_modify(&stream->sock_event.base);
}

static void on_transfer_error(x_event *ev)
{
	x_stream *stream = x_container_of(ev, x_stream, error_event.base);
	flush_all_send(stream, X_EPIPE);
	flush_all_recv(stream, X_EPIPE);
}

static void finish_stream(x_stream *stream, int send_error, int recv_error)
{
	x_sock_close(stream->sock_event.sock);
	x_reactor_remove(&stream->sock_event.base);
	if (stream->error_event.base.ev_flags & X_EV_REACTING) {
		x_reactor_remove(&stream->error_event.base);
		x_evtimer_init(&stream->error_event, 0, X_EV_ONCE, on_transfer_error);
	}
	stream->finished = true;
}

static void on_transfer_timeout_event(x_event *ev)
{
	x_stream *stream = x_container_of(ev, x_stream, error_event.base);
	flush_all_send(stream, X_ECONNRESET);
	flush_all_recv(stream, X_ECONNRESET);
	finish_stream(stream, X_ETIMEDOUT, X_ETIMEDOUT);
}

static void complate_transfer(x_stream *stream, struct transfer *p)
{
	p->on_transferred(0, p->transferred, p->user);
	x_list_del(&p->link);
	x_free(p);
	x_reactor_modify(&stream->error_event.base);
}

static void on_stream_sock_event(x_event *e)
{
	x_stream *stream = x_container_of(e, x_stream, sock_event.base);
	if (e->res_flags & X_EV_ERROR) {
		flush_all_send(stream, X_ECONNRESET);
		flush_all_recv(stream, X_ECONNRESET);
		finish_stream(stream, X_ECONNRESET, X_ECONNRESET);
	}
	if (e->res_flags & X_EV_READ) {
		struct transfer *p = x_container_of(x_list_first(&stream->recv_list), struct transfer, link);
		char *buf = (char *)p->iovec[p->iovec_index].base + p->iovec_shift;
		size_t buf_len = p->iovec[p->iovec_index].len - p->iovec_shift;
		x_ssize recv_len = recv(stream->sock_event.sock, buf, buf_len, 0);
		if (recv_len < 0) {
			stream->send_error = stream->recv_error = errno;
			flush_all_recv(stream, errno);
			return;
		}
		else if (recv_len == 0 && p->total_len != 0) {
			complate_transfer(stream, p);
			flush_all_recv(stream, 0);
			return;
		}
		p->transferred += recv_len;
		p->iovec_shift += recv_len;
		if (p->wait_all) {
			if (p->iovec_shift == p->iovec[p->iovec_index].len) {
				p->iovec_index++;
				if (p->iovec_index == p->iovec_count) {
					complate_transfer(stream, p);
				}
			}
		}
		else {
			complate_transfer(stream, p);
		}
		if (x_list_is_empty(&stream->recv_list)) {
			stream->sock_event.base.ev_flags &= ~X_EV_READ;
			x_reactor_modify(&stream->sock_event.base);
		}
	}
	if (e->res_flags & X_EV_WRITE) {
		struct transfer *p = x_container_of(x_list_first(&stream->send_list), struct transfer, link);
		char *buf = (char *)p->iovec[p->iovec_index].base + p->iovec_shift;
		size_t buf_len = p->iovec[p->iovec_index].len - p->iovec_shift;
		x_ssize send_len = send(stream->sock_event.sock, buf, buf_len, MSG_NOSIGNAL);
		if (send_len < 0) {
			stream->send_error = X_ENETRESET;
			flush_all_send(stream, errno);
			finish_stream(stream, X_ENETRESET, errno);
			return;
		}
		p->transferred += send_len;
		p->iovec_shift += send_len;
		if (p->wait_all) {
			if (p->iovec_shift == p->iovec[p->iovec_index].len) {
				p->iovec_index++;
				if (p->iovec_index == p->iovec_count) {
					complate_transfer(stream, p);
				}
			}
		}
		else {
			complate_transfer(stream, p);
		}
		if (x_list_is_empty(&stream->send_list)) {
			stream->sock_event.base.ev_flags &= ~X_EV_WRITE;
			x_reactor_modify(&stream->sock_event.base);
		}
	}
}

x_stream *x_stream_create(x_reactor *reactor, x_sock sock)
{
	x_stream *stream = x_malloc(NULL, sizeof *stream);
	x_mset_init(&stream->mset);
	x_list_init(&stream->send_list);
	x_list_init(&stream->recv_list);
	stream->reactor = reactor;
	stream->send_error = stream->recv_error = 0;
	x_evtimer_init(&stream->error_event, TRANSFER_TIMEOUT, 0, on_transfer_timeout_event);
	x_evsocket_init(&stream->sock_event, sock, 0, on_stream_sock_event);
	if (!x_reactor_add(reactor, &stream->sock_event.base)) {
		x_free(stream);
		return NULL;
	}
	if (x_sock_set_nonblocking(sock, true)) {
		x_reactor_remove(&stream->sock_event.base);
		x_free(stream);
		return NULL;
	}
	return stream;
}

void x_stream_free(x_stream *stream)
{
	if (!stream)
		return;
	flush_all_send(stream, X_ECANCELED);
	flush_all_recv(stream, X_ECANCELED);
	x_reactor_remove(&stream->sock_event.base);
	x_sock_close(stream->sock_event.sock);
	x_free(stream);
}

static int iovec_count(x_iovec iovec[])
{
	int i;
	for (i = 0; iovec[i].base; i++);
	return i;
}

static struct transfer *create_transfer(x_stream *stream, x_iovec iovec[], bool wait_all, x_stream_handler *on_transferred, void *user)
{
	int iovec_cnt = iovec_count(iovec);
	struct transfer *p = x_malloc(&stream->mset, sizeof *p + sizeof(x_iovec) * iovec_cnt);
	p->iovec_count = iovec_cnt;
	p->total_len = 0;
	p->iovec_index = 0;
	p->iovec_shift = 0;
	p->transferred = 0;
	p->wait_all = wait_all;
	p->on_transferred = on_transferred;
	p->user = user;
	p->error = 0;
	for (int i = 0; i < iovec_cnt; i++) {
		p->iovec[i] = iovec[i];
		p->total_len += iovec[i].len;
	}
	return p;
}

static void stream_write(x_stream *stream, bool wait_all, x_iovec iovec[], x_stream_handler *on_transferred, void *user)
{
	struct transfer *p = create_transfer(stream, iovec, wait_all, on_transferred, user);
	if (!wait_all) {
		x_link *last = x_list_last(&stream->send_list);
		if (last) {
			struct transfer *last_transfer = x_container_of(last, struct transfer, link);
			if (!last_transfer->wait_all)
				abort();
		}
	}
	x_list_add_back(&stream->send_list, &p->link);
	if (stream->finished) {
		if (!(stream->error_event.base.ev_flags & X_EV_REACTING))
			x_reactor_add(stream->reactor, &stream->error_event.base);
	}
	else {
		x_reactor_modify(&stream->error_event.base);
		if (!(stream->sock_event.base.ev_flags & X_EV_WRITE)) {
			stream->sock_event.base.ev_flags |= X_EV_WRITE;
			x_reactor_modify(&stream->sock_event.base);
		}
	}
}

static void stream_read(x_stream *stream, bool wait_all, x_iovec iovec[], x_stream_handler *on_transferred, void *user)
{
	struct transfer *p = create_transfer(stream, iovec, wait_all, on_transferred, user);
	if (!wait_all) {
		x_link *last = x_list_last(&stream->recv_list);
		if (last) {
			struct transfer *last_transfer = x_container_of(last, struct transfer, link);
			if (!last_transfer->wait_all)
				abort();
		}
	}
	x_list_add_back(&stream->recv_list, &p->link);
	if (stream->finished) {
		if (!(stream->error_event.base.ev_flags & X_EV_REACTING))
			x_reactor_add(stream->reactor, &stream->error_event.base);
	}
	else {
		x_reactor_modify(&stream->error_event.base);
		if (!(stream->sock_event.base.ev_flags & X_EV_READ)) {
			stream->sock_event.base.ev_flags |= X_EV_READ;
			x_reactor_modify(&stream->sock_event.base);
		}
	}
}

void x_stream_send(x_stream *stream, x_iovec iovec[], x_stream_handler *on_transfer, void *user)
{
	stream_write(stream, true, iovec, on_transfer, user);
}

void x_stream_recv(x_stream *stream, x_iovec iovec[], x_stream_handler *on_transfer, void *user)
{
	stream_read(stream, true, iovec, on_transfer, user);
}

void x_stream_send_buf(x_stream *stream, void *data, size_t size, x_stream_handler *on_transfer, void *user)
{
	x_iovec iovec[] = { { data, size }, { NULL } };
	stream_write(stream, false, iovec, on_transfer, user);
}

void x_stream_recv_buf(x_stream *stream, void *data, size_t size, x_stream_handler *on_transfer, void *user)
{
	x_iovec iovec[] = { { data, size }, { NULL } };
	stream_read(stream, false, iovec, on_transfer, user);
}

static void resolver_complete_event(x_event *ev)
{
	x_resolver *rvr = x_container_of(ev, x_resolver, complete_event);
	x_mutex_lock(&rvr->lock);
	while (!x_list_is_empty(&rvr->result_list)) {
		int n = 0;
		struct addrinfo *p;
		struct query *query = x_container_of(x_list_first(&rvr->result_list), struct query, link);
		x_list_del(&query->link);
		x_mutex_unlock(&rvr->lock);
		for (p = query->res; p; p = p->ai_next)
			n++;
		x_ipaddr *addrs = x_malloc(&rvr->mset, sizeof(x_ipaddr) * n);
		p = query->res;
		for (int i = 0; p; p = p->ai_next) {
			addrs[i].family = p->ai_family;
			if (p->ai_family == AF_INET)
				addrs[i++].un.in4 = ((struct sockaddr_in *)p->ai_addr)->sin_addr;
			else if (p->ai_family == AF_INET6)
				addrs[i++].un.in6 = ((struct sockaddr_in6 *)p->ai_addr)->sin6_addr;
		}
		query->on_resolve(query->error, query->name, addrs, n, query->user);
		freeaddrinfo(query->res);
		x_free(query);
		x_mutex_lock(&rvr->lock);
	}
	ev->res_flags = 0;
	x_mutex_unlock(&rvr->lock);
}

x_resolver *x_resolver_create(x_reactor *reactor)
{
	x_resolver *rvr = x_malloc(NULL, sizeof *rvr);
	x_evobject_init(&rvr->complete_event, 0, resolver_complete_event);
	if (x_reactor_add(reactor, &rvr->complete_event.base)) {
		x_free(rvr);
		return NULL;
	}
	if (x_thrpool_init(&rvr->thrpool, x_min(x_sys_nprocs(), 8))) {
		x_reactor_remove(&rvr->complete_event.base);
		x_free(rvr);
		return NULL;
	}
	x_mutex_init(&rvr->lock);
	x_list_init(&rvr->query_list);
	x_list_init(&rvr->result_list);
	rvr->reactor = reactor;
	x_mset_init(&rvr->mset);
	return 0;
}

void x_resolver_free(x_resolver *rvr)
{
	if (!rvr)
		return;
	x_thrpool_clear_work(&rvr->thrpool);
	x_thrpool_sync(&rvr->thrpool);
	x_thrpool_free(&rvr->thrpool);
	x_reactor_remove(&rvr->complete_event.base);
	x_mset_free(&rvr->mset);
	x_free(rvr);
}

static void resolve_handler(x_thrwork *work)
{
	struct query *query = x_container_of(work, struct query, work);
	struct addrinfo ai = {
		.ai_protocol = IPPROTO_TCP,
	};
	query->error = getaddrinfo(query->name, NULL, &ai, &query->res);
	x_mutex_lock(&query->rvr->lock);
	x_list_del(&query->link);
	x_list_add_back(&query->rvr->result_list, &query->link);
	query->rvr->complete_event.base.res_flags |= X_EV_READ;
	x_mutex_unlock(&query->rvr->lock);
	x_reactor_signal(query->rvr->reactor);
}

void x_resolver_query(x_resolver *rvr, const char *name, x_resolver_handler *on_resolve, void *user)
{
	struct query *query = x_malloc(&rvr->mset, sizeof *query + strlen(name) + 1);
	strcpy(query->name, name);
	query->on_resolve = on_resolve;
	query->user = user;
	query->rvr = rvr;
	x_thrwork_init(&query->work, resolve_handler);

	x_mutex_lock(&query->rvr->lock);
	x_list_add_back(&rvr->query_list, &query->link);
	x_mutex_unlock(&query->rvr->lock);

	x_thrpool_add_work(&rvr->thrpool, &query->work);
}

x_timer *x_timer_create(x_reactor *reactor)
{
	x_timer *timer = x_malloc(NULL, sizeof *timer);
	x_uidpool_init(&timer->uidpool);
	timer->reactor = reactor;
	x_mset_init(&timer->mset);
	return timer;
}

static void free_clock(struct clock *c)
{
	if (c->timer_event.base.ev_flags & X_EV_REACTING)
		x_reactor_remove(&c->timer_event.base);
	x_uidpool_put(&c->timer->uidpool, &c->uid);
	x_free(c);
}

void x_timer_free(x_timer *timer)
{
	if (!timer)
		return;
	x_sklist_foreach(cur, &timer->uidpool.uid_set)
		free_clock(cur);
	x_uidpool_free(&timer->uidpool);
	x_mset_free(&timer->mset);
	x_free(timer);
}

static void on_periodic_timedout(x_event *ev)
{
	struct clock *c = x_container_of(ev, struct clock, timer_event.base);
	if (c->on_timedout(c->uid.num, c->user))
		free_clock(c);
}

static void on_first_timedout(x_event *ev)
{
	struct clock *c = x_container_of(ev, struct clock, timer_event.base);
	if (c->on_timedout(c->uid.num, c->user)) {
		free_clock(c);
		return;
	}
	if (c->interval <= 0) {
		free_clock(c);
		return;
	}
	x_evtimer_init(&c->timer_event, c->interval, 0, on_periodic_timedout);
	x_reactor_add(c->timer->reactor, &c->timer_event.base);
}

uint32_t x_timer_add(x_timer *timer, int delay_ms, int interval_ms, x_timer_handler *on_timedout, void *user)
{
	if (delay_ms < 0 && interval_ms <= 0)
		return X_BADUID;
	struct clock *c = x_malloc(&timer->mset, sizeof *c);
	if (x_uidpool_get(&timer->uidpool, &c->uid)) {
		x_free(c);
		return X_BADUID;
	}
	c->interval = interval_ms;
	c->on_timedout = on_timedout;
	c->user = user;
	c->timer = timer;
	if (delay_ms >= 0)
		x_evtimer_init(&c->timer_event, delay_ms, X_EV_ONCE, on_first_timedout);
	else
		x_evtimer_init(&c->timer_event, interval_ms, 0, on_periodic_timedout);
	x_reactor_add(timer->reactor, &c->timer_event.base);
	return c->uid.num;
}

void x_timer_remove(x_timer *timer, uint32_t id)
{
	x_uidlink *uid = x_uidpool_find(&timer->uidpool, id);
	if (!uid)
		return;
	struct clock *c = x_container_of(uid, struct clock, uid);
	free_clock(c);
}

