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

#include "x/event.h"
#include <stdio.h>

void x_evsocket_init(x_evsocket *event, x_sock sock, short flags, void *data)
{
	memset(event, 0, sizeof *event);
	event->sock = sock;
	event->base.ev_flags = flags;
	event->base.type = X_EVENT_SOCKET;
	event->base.data = data;
}


void x_evtimer_init(x_evtimer *event, int interval_ms, short flags, void *data)
{
	memset(event, 0, sizeof *event);
	event->interval = interval_ms;
	event->base.ev_flags = flags;
	event->base.type = X_EVENT_TIMER;
	event->base.data = data;
}

void x_evobject_init(x_evobject *event, short flags, void *data)
{
	memset(event, 0, sizeof *event);
	event->base.ev_flags = flags;
	event->base.type = X_EVENT_OBJECT;
	event->base.data = data;
}

