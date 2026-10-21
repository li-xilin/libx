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
#include "x/invoke.h"
#include "x/reactor.h"

struct sync_call
{
	x_evobject event;
	x_invoke_handler_fn *handler;
	void *param;
	void *data;
	int retval;
};

static void sync_call_handler(x_event *e)
{
	struct sync_call *call = x_container_of(e, struct sync_call, event.base);
	call->retval = call->handler(call->param, call->data);
	e->res_flags = 0;
}

int x_invoke_sync(x_reactor *r, x_invoke_handler_fn *handler, void *param, void *data)
{
	struct sync_call call = {
		.event = X_EVOBJECT_INIT(X_EV_READ, sync_call_handler),
		.handler = handler,
		.param = param,
		.data = data,
		.retval = 0,
	};
	call.event.base.res_flags = X_EV_READ;
	if (x_reactor_add(r, &call.event.base))
		return -1;
	if (x_evobject_wait(&call.event, -1)) {
		x_reactor_remove(&call.event.base);
		return -1;
	}
	x_reactor_remove(&call.event.base);
	return call.retval;
}

