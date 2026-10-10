/*
 * Copyright (c) 2025 Li Xilin <lixilin@gmx.com>
 * 
 * Permission is hereby granted, free of charge, to one person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#ifndef X_EVENT_H
#define X_EVENT_H

#include "linkage.h"
#include "socket.h"
#include "time.h"
#include "list.h"
#include "heap.h"
#include "hmap.h"
#include <stdint.h>

X_EXTERN_C_BEGIN

#define X_EV_REACTING (1 << 0)
#define X_EV_ERROR    (1 << 1)
#define X_EV_ONCE     (1 << 2)
#define X_EV_READ     (1 << 3)
#define X_EV_WRITE    (1 << 4)
#define X_EV_ACCURATE (1 << 5)

#define X_EVENT_INIT(_type, _flags, _handler) {  \
	_type, \
	0, \
	_flags, \
	0, \
	_handler, \
}

#define X_EVSOCKET_INIT(_fd, _flags, _handler) { \
	X_EVENT_INIT(X_EVENT_SOCKET, _flags, _handler), \
	X_HNODE_INIT, \
	_fd, \
}

#define X_EVTIMER_INIT(_interval_ms, _flags, _handler) { \
	X_EVENT_INIT(X_EVENT_TIMER,_flags, _handler), \
	.interval = _interval_ms, \
}

#define X_EVOBJECT_INIT(_flags, _handler) { \
	X_EVENT_INIT(X_EVENT_OBJECT, _flags, _handler), \
}

enum {
	X_EVENT_SOCKET,
	X_EVENT_TIMER,
	X_EVENT_OBJECT,
};

typedef void x_event_handler(x_event *e);

struct x_event_st
{
	uint16_t type;
	uint16_t label;
	short ev_flags, res_flags;
	x_event_handler *handler;
	x_link event_link;
	x_link pending_link;
	x_reactor *reactor;
};

struct x_evsocket_st
{
	x_event base;
	x_hnode hash_node;
	x_sock sock;
};

struct x_evtimer_st
{
	x_event base;
	size_t interval;
	x_ranode node;
	struct timeval expiration;
};

struct x_evobject_st
{
	x_event base;
	x_link link;
};

void x_evsocket_init(x_evsocket *event, x_sock sock, short flags, x_event_handler *handler);
void x_evtimer_init(x_evtimer *event, int interval_ms, short flags, x_event_handler *handler);
void x_evobject_init(x_evobject *event, short flags, x_event_handler *handler);

X_EXTERN_C_END

#endif

