/*
 * Copyright (c) 2022-2023,2025 Li Xilin <lixilin@gmx.com>
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

#ifndef X_REACTOR_H
#define X_REACTOR_H

#include "socket.h"
#include "event.h"
#include "list.h"
#include "heap.h"
#include "hmap.h"
#include "mutex.h"
#include "socket.h"
#include "event.h"

struct x_reactor_st
{
	x_list pending_list;
	x_mutex lock;
	x_evsocket io_event;
	x_sock io_pipe1;
	bool breaking;

	const struct x_sockmux_ops_st *mux_ops;
	x_sockmux *mux;
	x_hmap sock_ht;

	x_heap timer_heap;
	struct timeval last_wait;

	x_list obj_list;
};

int x_reactor_init(x_reactor *r);
void x_reactor_clear(x_reactor *r);
void x_reactor_free(x_reactor *r);
int x_reactor_add(x_reactor *r, x_event *e);
void x_reactor_pend(x_reactor *r, x_event *e, short res_flags);
void x_reactor_remove(x_reactor *r, x_event *e);
int x_reactor_wait(x_reactor *r);
void x_reactor_signal(x_reactor *r);
void x_reactor_break(x_reactor *r);
x_event *x_reactor_pop_event(x_reactor *r);

#endif
