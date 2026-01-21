/*
 * Copyright (c) 2022-2025 Li Xilin <lixilin@gmx.com>
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

#include "x/sockmux.h"
#include "x/reactor.h"
#include "x/string.h"
#include "x/event.h"
#include "x/socket.h"
#include "x/time.h"
#include "x/log.h"
#include "x/reactor.h"
#include "x/errno.h"
#ifdef X_OS_WIN32
#include <windows.h>
#else
#include <unistd.h>
#include <sys/socket.h>
#endif
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

static void ioevent_reset(x_reactor *r);
static void ioevent_set(x_reactor *r);

static void reset_all_timer(x_reactor *r, struct timeval *now)
{
	for (int i = 0; i < r->timer_heap.entry_cnt; i++) {
		x_evtimer *e = x_container_of(r->timer_heap.table[i], x_evtimer, node);
		e->expiration = *now;
	}
}

static int reactor_pend_timer(x_reactor *r, const struct timeval *tv)
{
	int npendings = 0;
	struct timeval now;
	if (x_time_now(&now))
		return 0;
	int ntimers = r->timer_heap.entry_cnt;
	for (int i = 0; i < ntimers; i++) {
		x_evtimer *top = x_container_of(x_heap_top(&r->timer_heap), x_evtimer, node);
		if (x_time_lt(now, top->expiration))
			break;
		x_heap_pop(&r->timer_heap);
		if (!top->base.pending_link.next)
			x_list_add_back(&r->pending_list, &top->base.pending_link);
		if (!(top->base.ev_flags & X_EV_ONCE)) {
			if (top->base.ev_flags & X_EV_ACCURATE) {
				struct timeval diff;
				x_time_diff(now, top->expiration, &diff);
				x_time_forward(top->expiration, (x_time_to_msec(diff) / top->interval + 1) * top->interval);
			}
			else {
				top->expiration = now;
				x_time_forward(top->expiration, top->interval);
			}
			x_heap_push(&r->timer_heap, &top->node);
		}
		else
			top->base.ev_flags &= ~X_EV_REACTING;
		npendings += 1;
	}
	return npendings;
}

static int reactor_pend_socket(x_reactor *r)
{
	x_sock fd;
	short flags;
	int npendings = 0;
	while ((fd = r->mux_ops->m_next(r->mux, &flags)) != X_BADSOCK) {
		x_evsocket pattern = { .sock = fd };
		x_link *link = x_hmap_find(&r->sock_ht, &pattern.hash_link);
		assert(link);
		x_evsocket *e = x_container_of(link, x_evsocket, hash_link);
		e->base.res_flags = flags;
		if (e == &r->io_event) {
			ioevent_reset(r);
			continue;
		}
		if (!e->base.pending_link.next)
			x_list_add_back(&r->pending_list, &e->base.pending_link);
		npendings++;
	}
	return npendings;
}

static int reactor_pend_object(x_reactor *r)
{
	int npendings = 0;
	x_list_foreach(cur, &r->obj_list) {
		x_evobject *e = x_container_of(cur, x_evobject, link);
		if (!e->base.res_flags)
			continue;
		if (!e->base.pending_link.next)
			x_list_add_back(&r->pending_list, &e->base.pending_link);
		npendings += 1;
	}
	return npendings;
}

void x_reactor_break(x_reactor *r)
{
	x_mutex_lock(&r->lock);
	r->breaking = true;
	x_mutex_unlock(&r->lock);
	ioevent_set(r);
}

static size_t evsocket_hash(const x_link *node)
{
	x_evsocket *e = x_container_of(node, x_evsocket, hash_link);
	return x_memhash(&e->sock, sizeof e->sock);
}
static bool evsocket_equal(const x_link *node1, const x_link *node2)
{
	x_evsocket *e1 = x_container_of(node1, x_evsocket, hash_link);
	x_evsocket *e2 = x_container_of(node2, x_evsocket, hash_link);
	return e1->sock == e2->sock;
}

static bool timer_ordered(const x_ranode *p1, const x_ranode *p2)
{
	const x_evtimer *e1 = x_container_of(p1, x_evtimer, node);
	const x_evtimer *e2 = x_container_of(p2, x_evtimer, node);
	return x_time_le(e1->expiration, e2->expiration);
}

int x_reactor_init(x_reactor *r)
{
	x_sock pair[2] = { -1, -1 };
	memset(r, 0, sizeof(x_reactor));

	r->mux_ops = &x_sockmux_epoll;
	r->mux = r->mux_ops->m_create();
	if (!r->mux)
		goto fail;
	if (x_sock_pair(AF_UNIX, SOCK_STREAM, 0, pair) == -1)
		goto fail;
	x_mutex_init(&r->lock);
	x_hmap_init(&r->sock_ht, 0.5, evsocket_hash, evsocket_equal);
	x_list_init(&r->pending_list);
	x_list_init(&r->obj_list);
	x_heap_init(&r->timer_heap, timer_ordered);
	x_evsocket_init(&r->io_event, pair[0], X_EV_READ, NULL);
	x_reactor_add(r, &r->io_event.base);
	r->breaking = false;
	r->io_pipe1 = pair[1];
	return 0;
fail:
	if (pair[0] != -1) {
		x_sock_close(pair[0]);
		x_sock_close(pair[1]);
	}
	if (r->mux)
		r->mux_ops->m_free(r->mux);
	return -1;
}

static void reactor_clean_events(x_reactor *r)
{
	x_evsocket *e;
	for (int i = 0; i < r->sock_ht.slot_cnt; i++) {
		x_list_popeach(cur, &r->sock_ht.table[i]) {
			e = x_container_of(cur, x_evsocket, hash_link);
			r->mux_ops->m_del(r->mux, e->sock, e->base.ev_flags);
		}
	}
	x_heap_free(&r->timer_heap);
	x_list_popeach(cur, &r->obj_list);
}

void x_reactor_clear(x_reactor *r)
{
	x_mutex_lock(&r->lock);
	reactor_clean_events(r);
	x_reactor_add(r, &r->io_event.base);
	x_mutex_unlock(&r->lock);
}

void x_reactor_free(x_reactor *r)
{
	reactor_clean_events(r);
	r->mux_ops->m_free(r->mux);
	x_sock_close(r->io_event.sock);
	x_sock_close(r->io_pipe1);
	x_mutex_destroy(&r->lock);
	x_hmap_free(&r->sock_ht);
}

int x_reactor_add(x_reactor *r, x_event *e)
{
	int retval = -1;
	assert(r != NULL && e != NULL);
	if (e->ev_flags & X_EV_REACTING) {
		errno = X_EALREADY;
		return -1;
	}
	x_evsocket *esock = x_container_of(e, x_evsocket, base);
	x_evobject *eobj = x_container_of(e, x_evobject, base);
	x_evtimer *etimer = x_container_of(e, x_evtimer, base);
	x_mutex_lock(&r->lock);
	switch (e->type) {
		case X_EVENT_TIMER:
			 x_time_now(&etimer->expiration);
			 x_time_forward(etimer->expiration, etimer->interval);
			x_heap_push(&r->timer_heap, &etimer->node);
			break;
		case X_EVENT_SOCKET:
			if (x_hmap_find_or_insert(&r->sock_ht, &esock->hash_link)) {
				errno = X_EEXIST;
				goto out;
			}
			if (r->mux_ops->m_add(r->mux, esock->sock, e->ev_flags) == -1) {
				x_hmap_remove(&r->sock_ht, &esock->hash_link);
				goto out;
			}
			break;
		case X_EVENT_OBJECT:
			x_list_add_back(&r->obj_list, &eobj->link);
			break;
		default:
			errno = X_EINVAL;
			goto out;
	}
	e->reactor = r;
	e->ev_flags |= X_EV_REACTING;
	ioevent_set(r);
	retval = 0;
out:
	x_mutex_unlock(&r->lock);
	return retval;
}

int x_reactor_modify(x_event *e)
{
	int retval = -1;
	assert(e != NULL);
	if (!(e->ev_flags & X_EV_REACTING)) {
		errno = X_EINVAL;
		return -1;
	}
	x_reactor *r = e->reactor;
	x_evsocket *esock = x_container_of(e, x_evsocket, base);
	x_evtimer *etimer = x_container_of(e, x_evtimer, base);
	x_mutex_lock(&r->lock);
	switch (e->type) {
		case X_EVENT_TIMER:
			x_heap_remove(&r->timer_heap, &etimer->node);
			x_heap_push(&r->timer_heap, &etimer->node);
			break;
		case X_EVENT_SOCKET:
			if (r->mux_ops->m_mod(r->mux, esock->sock, e->ev_flags) == -1)
				goto out;
			break;
		case X_EVENT_OBJECT:
			break;
		default:
			errno = X_EINVAL;
			goto out;
	}
	ioevent_set(r);
	retval = 0;
out:
	x_mutex_unlock(&r->lock);
	return retval;
}

void x_reactor_pend(x_reactor *r, x_event *e, short res_flags)
{
	assert(r != NULL && e != NULL);
	assert(e->ev_flags & X_EV_REACTING);
	e->res_flags = res_flags;
	x_list_add_back(&r->pending_list, &e->pending_link);
}

void x_reactor_remove(x_reactor *r, x_event *e)
{
	assert(r != NULL && e != NULL);
	assert(e->ev_flags & X_EV_REACTING);
	x_evsocket *esock = x_container_of(e, x_evsocket, base);
	x_evobject *eobj = x_container_of(e, x_evobject, base);
	x_evtimer *etimer = x_container_of(e, x_evtimer, base);
	x_mutex_lock(&r->lock);
	switch (e->type) {
		case X_EVENT_TIMER:
			x_heap_remove(&r->timer_heap, &etimer->node);
			break;
		case X_EVENT_SOCKET:
			if (x_hmap_find_and_remove(&r->sock_ht, &esock->hash_link)) {
				r->mux_ops->m_del(r->mux, esock->sock, e->ev_flags);
				goto out;
			}
			break;
		case X_EVENT_OBJECT:
			x_list_del(&eobj->link);
			break;
		default:
			errno = X_EINVAL;
			goto out;
	}
	e->reactor = NULL;
	e->ev_flags &= ~X_EV_REACTING;
	ioevent_set(r);
out:
	x_mutex_unlock(&r->lock);
}

static void ioevent_reset(x_reactor *r)
{
	char buf[1024];
	while (recv(r->io_event.sock, buf, sizeof buf, 0) == sizeof buf);
}

static void ioevent_set(x_reactor *r)
{
	char octet = 0;
	send(r->io_pipe1, &octet, sizeof(octet), 0);
}

int x_reactor_wait(x_reactor *r)
{
	assert(r != NULL);
	struct timeval tv, *ptv = NULL;
	int npendings = -1;
	x_mutex_lock(&r->lock);
	if (r->breaking) {
		r->breaking = false;
		x_mutex_unlock(&r->lock);
		return 0;
	}
	do {
		struct timeval now;
		x_time_now(&now);
		x_ranode *timer_node = x_heap_top(&r->timer_heap);
		if (timer_node) {
			x_evtimer *evt = x_container_of(timer_node, x_evtimer, node);
			if (x_time_lt(evt->expiration, now)) {
				x_time_set_msec(tv, 0);
			}
			else
				x_time_diff(evt->expiration, now, &tv);
			
			if (x_time_to_msec(tv) > evt->interval + 5000) {
				reset_all_timer(r, &now);
				x_time_set_msec(tv, 0);
			}
			ptv = &tv;
		}
		x_mutex_unlock(&r->lock);
		int nreadys = r->mux_ops->m_poll(r->mux, ptv);
		x_mutex_lock(&r->lock);
		if (nreadys < 0) {
			npendings = -1;
			goto out;
		}
		npendings = 0;
		if (timer_node)
			npendings += reactor_pend_timer(r, &now);
		npendings += reactor_pend_socket(r);
		npendings += reactor_pend_object(r);
	} while (!r->breaking && !npendings);
	if (!npendings)
		r->breaking = false;
out:
	x_mutex_unlock(&r->lock);
	return npendings;
}


void x_reactor_signal(x_reactor *r)
{
	assert(r != NULL);
	ioevent_set(r);
}

x_event *x_reactor_pop_event(x_reactor *r)
{
	if (x_list_is_empty(&r->pending_list))
		return NULL;
	x_event *e = x_container_of(x_list_first(&r->pending_list), x_event, pending_link);
	x_list_del(&e->pending_link);
	return e;
}


