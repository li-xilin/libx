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
#ifndef X_ASYNET_H
#define X_ASYNET_H

#include "linkage.h"
#include "types.h"

X_EXTERN_C_BEGIN

#define X_RESOLVER_NAME_MAXLEN 255

typedef void x_dialer_connected_fn(int err, x_sock fd, void *user);
x_dialer *x_dialer_create(x_reactor *reactor);
void x_dialer_free(x_dialer *dialer);
void x_dialer_connect(x_dialer *dialer, const x_hostport *addr, int timeout_ms, x_dialer_connected_fn *on_connected, void *user);
void x_dialer_cancel(x_dialer *dialer);

typedef void x_listener_accepted_fn(x_sock fd, void *user);
x_listener *x_listener_create(x_reactor *reactor, const x_hostport *addr, x_listener_accepted_fn *on_accepted, void *user);
void x_listener_free(x_listener *lnr);
short x_listener_port(x_listener *lnr);
int x_listener_start(x_listener *lnr);
void x_listener_stop(x_listener *lnr);

typedef void x_stream_transferred_fn(int err, size_t transfered, void *user);
typedef void x_stream_finished_fn(x_stream *stream, void *user);
x_stream *x_stream_create(x_reactor *reactor, x_sock sock, int send_timeout, int recv_timeout, x_stream_finished_fn *on_finished, void *user);
x_sock x_stream_sock(x_stream *stream);
void x_stream_free(x_stream *stream);
int x_stream_send(x_stream *stream, x_iovec iovec[], x_stream_transferred_fn *on_sent, void *user);
int x_stream_recv(x_stream *stream, x_iovec iovec[], x_stream_transferred_fn *on_received, void *user);
int x_stream_send_buf(x_stream *stream, void *data, size_t size, x_stream_transferred_fn *on_sent, void *user);
int x_stream_recv_buf(x_stream *stream, void *data, size_t size, x_stream_transferred_fn *on_received, void *user);

typedef void x_resolver_completed_fn(int err, const char *name, x_ipaddr *addrs, size_t n, void *user);
x_resolver *x_resolver_create(x_reactor *reactor);
void x_resolver_free(x_resolver *rvr);
int x_resolver_query(x_resolver *rvr, const char *name, x_resolver_completed_fn *on_resolve, void *user);

typedef bool x_timer_timeout_fn(uint32_t id, void *user);
x_timer *x_timer_create(x_reactor *reactor);
void x_timer_free(x_timer *timer);
uint32_t x_timer_add(x_timer *timer, int delay_ms, int interval_ms, x_timer_timeout_fn *on_timedout, void *user);
void x_timer_remove(x_timer *timer, uint32_t id);

X_EXTERN_C_END

#endif

