/*
 * Copyright (c) 2026 Li Xilin <lixilin@gmx.com>
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
#include "sockmux.h"
#include "x/event.h"
#include "x/errno.h"
#include <poll.h>
#include <string.h>

#define INITIAL_EVENT_COUNT 32

struct x_sockmux_st
{
	int max_events;
	int n_events;
	int need_realoc;
	int need_rememcp;
	struct pollfd *fds;
	struct pollfd *fds_in; // to avoid competion
	int iterator;
	int nreadys;
};

static void poll_resize(x_sockmux * mux, int size)
{
	struct pollfd * pfds;
	x_assert(mux != NULL);
	pfds = x_realloc(mux->fds_in, size * sizeof(struct pollfd));
	mux->fds_in = pfds;
	mux->max_events = size;
	mux->need_realoc = 1;
	mux->need_rememcp = 1;
}

static x_sockmux *poll_mux_create(void)
{
	x_sockmux * ret = x_malloc(NULL, sizeof(x_sockmux));
	memset(ret, 0, sizeof(x_sockmux));
	poll_resize(ret, INITIAL_EVENT_COUNT);
	return ret;
}

static void poll_mux_free(x_sockmux *mux)
{
	if (!mux)
		return;
	if(mux->fds)
		x_free(mux->fds);
	if(mux->fds_in)
		x_free(mux->fds_in);
	x_free(mux);
}

static inline int poll_setup_mask(short flags)
{
	int ret = 0;
	if(flags & X_EV_READ)
		ret |= POLLIN | POLLPRI;
	if(flags & X_EV_WRITE)
		ret |= POLLOUT;
	return ret;
}

static int poll_mux_add(x_sockmux *mux, x_sock fd, short flags)
{
	int i;
	x_assert(mux);
	for(i = 0; i < mux->n_events; ++i) {
		if(fd == mux->fds_in[i].fd) {
			/* Override the existing event */
			mux->fds_in[i].events = poll_setup_mask(flags);
			mux->need_rememcp = 1;;
			return 0;
		}
	}
	if(mux->n_events >= mux->max_events)
		poll_resize(mux, mux->max_events << 1);
	mux->fds_in[i].fd = fd;
	mux->fds_in[i].events = poll_setup_mask(flags);
	mux->need_rememcp = 1;;
	++mux->n_events;
	return 0;
}

static int poll_mux_mod(x_sockmux *mux, x_sock fd, short flags)
{
	x_assert(mux);
	for(int i = 0; i < mux->n_events; ++i) {
		if(fd == mux->fds_in[i].fd) {
			mux->fds_in[i].events = poll_setup_mask(flags);
			mux->need_rememcp = 1;
			return 0;
		}
	}
	errno = X_ENOENT;
	return -1;
}

static int poll_mux_del(x_sockmux *mux, x_sock fd)
{
	x_assert(mux);
	for(int i = 0; i < mux->n_events; i++)
		if(mux->fds_in[i].fd == fd) {
			x_swap(mux->fds_in + i, mux->fds_in + mux->n_events - 1, struct pollfd);
			mux->need_rememcp = 1;
			mux->n_events --;
			break;
		}
	return 0;
}

static int poll_mux_poll(x_sockmux *mux, struct timeval *timeout)
{
	x_assert(mux);
	mux->iterator = 0;
	mux->nreadys = 0;
	if(mux->need_realoc) {
		mux->fds = x_realloc(mux->fds, mux->max_events * sizeof(struct pollfd));
		mux->need_realoc = 0;
	}
	if(mux->need_rememcp) {
		memcpy(mux->fds, mux->fds_in, mux->max_events * sizeof(struct pollfd));
		mux->need_rememcp = 0;
	}
	int nreadys = poll(mux->fds, mux->n_events, 
			timeout ? timeout->tv_sec * 1000 + timeout->tv_usec / 1000 : -1);
	if (nreadys < 0)
		return nreadys;
	mux->nreadys = nreadys;
	return nreadys;
}

static x_sock poll_mux_next(x_sockmux *mux, short *res_flags)
{
	x_assert(mux);
	if (!mux->nreadys)
		return X_BADSOCK;
	*res_flags = 0;
	for (int i = mux->iterator; i < mux->n_events; i++) {
		if (mux->fds[i].revents & (POLLIN | POLLPRI))
			*res_flags |= X_EV_READ;
		if (mux->fds[i].revents & POLLOUT)
			*res_flags |= X_EV_WRITE;
		if (mux->fds[i].revents & POLLERR)
			*res_flags |= X_EV_ERROR;
		if (mux->fds[i].revents & POLLNVAL)
			*res_flags |= X_EV_ERROR;
		if (*res_flags) {
			mux->iterator = i + 1;
			return mux->fds[i].fd;
		}
	}
	mux->iterator = mux->max_events;
	return X_BADSOCK;
}

const struct x_sockmux_ops_st g_sockmux_ops = {
	.m_add = poll_mux_add,
	.m_mod = poll_mux_mod,
	.m_del = poll_mux_del,
	.m_create = poll_mux_create,
	.m_free = poll_mux_free,
	.m_poll = poll_mux_poll,
	.m_next = poll_mux_next,
};

