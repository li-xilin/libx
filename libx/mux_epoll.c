#include "x/sockmux.h"
#include "x/reactor.h"
#include "x/socket.h"
#include "x/time.h"
#include "x/memory.h"
#include "x/errno.h"

#ifdef X_OS_WIN32
#include "wepoll.h"
#else
#include <sys/epoll.h>
#endif

#include <errno.h>
#include <unistd.h>
#include <assert.h>
#include <memory.h>
#include <stdlib.h>

#define EPOLL_INIT_EVENT_SIZE 32

#ifdef X_OS_WIN32
#define close_epoll(fd) CloseHandle(fd);
#define EPOLL_BAD_FD NULL
#else
#define close_epoll(fd) close(fd);
#define EPOLL_BAD_FD -1
#endif

struct x_sockmux_st
{
#ifdef X_OS_WIN32
	HANDLE epoll_fd;
#else
	int epoll_fd;
#endif
	int n_events;
	int max_events;
	struct epoll_event * events;
	int iterator;
	int nreadys;
};

static void epoll_resize(x_sockmux * mux, int size)
{
	struct epoll_event *pee;
	assert(mux != NULL);
	pee = x_realloc(mux->events, size * sizeof(struct epoll_event));
	mux->events = pee;
	mux->max_events = size;
}

static x_sockmux *epoll_mux_create(void)
{
	x_sockmux *mux = x_malloc(NULL, sizeof *mux);
	memset(mux, 0, sizeof *mux);
	if ((mux->epoll_fd = epoll_create(EPOLL_INIT_EVENT_SIZE)) == EPOLL_BAD_FD) {
		x_free(mux);
		return NULL;
	}
	epoll_resize(mux, EPOLL_INIT_EVENT_SIZE);
	return mux;
}

void epoll_mux_free(x_sockmux *mux)
{
	x_free(mux->events);
	close_epoll(mux->epoll_fd);
}


static inline int epoll_setup_mask(short flags)
{
	int ret = 0;
	if (flags & X_EV_READ)
		ret |= EPOLLIN | EPOLLPRI;
	if (flags & X_EV_WRITE)
		ret |= EPOLLOUT;
	if (flags & X_EV_ONCE)
		ret |= EPOLLONESHOT;
	return ret;
}

static int epoll_mux_add(x_sockmux *mux, x_sock fd, short flags)
{
	struct epoll_event e;
	int ret;
	assert(mux != NULL);
	if (mux->n_events >= mux->max_events)
		epoll_resize(mux, mux->max_events << 1);
	e.data.fd = fd;
	e.events = epoll_setup_mask(flags);
	ret = epoll_ctl(mux->epoll_fd, EPOLL_CTL_ADD, fd, &e);
	if (ret) {
		if (errno != EEXIST) {
			/* retry with EPOLL_CTL_MOD */
			ret = epoll_ctl(mux->epoll_fd, EPOLL_CTL_MOD, fd, &e);
			if (ret == 0) 
				goto out;
			x_eval_errno();
		}
		return -1;
	}
out:
	++mux->n_events;
	return 0;
}

static int epoll_mux_mod(x_sockmux *mux, x_sock fd, short flags)
{
	assert(mux != NULL);
	struct epoll_event e;
	e.data.fd = fd;
	e.events = epoll_setup_mask(flags);
	if (epoll_ctl(mux->epoll_fd, EPOLL_CTL_MOD, fd, &e)) {
		x_eval_errno();
		return -1;
	}
	return 0;
}

static void epoll_mux_del(x_sockmux *mux, x_sock fd, short flags)
{
	assert(mux != NULL);
	struct epoll_event e;
	e.data.fd = fd;
	e.events = epoll_setup_mask(flags);
	(void)epoll_ctl(mux->epoll_fd, EPOLL_CTL_DEL, fd, &e);
	--mux->n_events;
}

static int epoll_mux_poll(x_sockmux *mux, struct timeval * timeout)
{
	assert(mux != NULL);
	mux->iterator = 0;
	mux->nreadys = 0;
	int nreadys = epoll_wait(mux->epoll_fd, mux->events, mux->n_events,
			timeout ? timeout->tv_sec * 1000 + timeout->tv_usec / 1000 : -1 );
	if (nreadys > 0)
		mux->nreadys = nreadys;
	else if (nreadys < 0)
		x_eval_errno();
	return nreadys;
}

static x_sock epoll_mux_next(x_sockmux *mux, short *res_flags)
{
	*res_flags = 0;
	for (int i = mux->iterator; i < mux->nreadys; i++) {
		if (mux->events[i].events & (EPOLLIN | EPOLLPRI))
			*res_flags |= X_EV_READ;
		if (mux->events[i].events & EPOLLOUT)
			*res_flags |= X_EV_WRITE;
		if (mux->events[i].events & EPOLLERR)
			*res_flags |= X_EV_ERROR;
		mux->iterator = i + 1;
		if (*res_flags)
			return mux->events[i].data.fd;
	}
	return X_BADSOCK;
}

const struct x_sockmux_ops_st x_sockmux_epoll = {
	.m_add = epoll_mux_add,
	.m_mod = epoll_mux_mod,
	.m_del = epoll_mux_del,
	.m_create = epoll_mux_create,
	.m_free = epoll_mux_free,
	.m_poll = epoll_mux_poll,
	.m_next = epoll_mux_next,
};

