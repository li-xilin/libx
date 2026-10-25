#include "sockmux.h"
#include "x/reactor.h"
#include "x/socket.h"
#include "x/time.h"
#include "x/memory.h"
#include "x/errno.h"
#include <string.h>
#include <sys/event.h>
#include <sys/time.h>
#include <unistd.h>

#define KQUEUE_INIT_EVENT_SIZE 32

#define close_kqueue(fd) close(fd)
#define KQUEUE_BAD_FD -1

struct x_sockmux_st {
	int kq_fd;
	int n_events;      /* number of currently registered fds */
	int max_events;    /* capacity of the events array */
	struct kevent *events;
	int iterator;
	int nreadys;
};

static void kqueue_resize(x_sockmux *mux, int size)
{
	struct kevent *pev;
	x_assert(mux != NULL);
	pev = x_realloc(mux->events, size * sizeof(struct kevent));
	mux->events = pev;
	mux->max_events = size;
}

static x_sockmux *kqueue_mux_create(void)
{
	x_sockmux *mux = x_malloc(NULL, sizeof *mux);
	memset(mux, 0, sizeof *mux);
	if ((mux->kq_fd = kqueue()) == KQUEUE_BAD_FD) {
		x_free(mux);
		return NULL;
	}
	kqueue_resize(mux, KQUEUE_INIT_EVENT_SIZE);
	return mux;
}

static void kqueue_mux_free(x_sockmux *mux)
{
	x_free(mux->events);
	close_kqueue(mux->kq_fd);
	x_free(mux);
}

/*
 * Translate flags into kevent filters/fflags.
 * kqueue requires separate filters for read and write, whereas epoll
 * can express both with a single event. This implementation uses a
 * "delete old filters, then add new ones" strategy on every mod.
 *
 * Returns 0 on success, -1 on failure.
 */
static int kqueue_apply(x_sockmux *mux, x_sock fd, short flags, int is_add)
{
	struct kevent changes[2];
	int nchanges = 0;
	int nfilters = 0;
	int filters[2];
	int i;

	/* Determine which filters need to be registered */
	if (flags & X_EV_READ)
		filters[nfilters++] = EVFILT_READ;
	if (flags & X_EV_WRITE)
		filters[nfilters++] = EVFILT_WRITE;

	if (is_add) {
		for (i = 0; i < nfilters; i++) {
			unsigned short fflags = 0;
			if (flags & X_EV_ONCE)
				fflags |= EV_ONESHOT;
			EV_SET(&changes[nchanges], fd, filters[i], EV_ADD | fflags, 0, 0, NULL);
			nchanges++;
		}
	} else {
		/*
		 * MOD: first try to delete both filters, then re-add the needed ones.
		 * Note: deleting a non-existent filter returns ENOENT in kqueue,
		 * so the EV_DELETE calls are issued but individual errors are
		 * ignored; the overall kevent call decides success.
		 */
		EV_SET(&changes[nchanges], fd, EVFILT_READ, EV_DELETE, 0, 0, NULL);
		nchanges++;
		EV_SET(&changes[nchanges], fd, EVFILT_WRITE, EV_DELETE, 0, 0, NULL);
		nchanges++;
		kevent(mux->kq_fd, changes, nchanges, NULL, 0, NULL);
		nchanges = 0;
		for (i = 0; i < nfilters; i++) {
			unsigned short fflags = 0;
			if (flags & X_EV_ONCE)
				fflags |= EV_ONESHOT;
			EV_SET(&changes[nchanges], fd, filters[i], EV_ADD | fflags, 0, 0, NULL);
			nchanges++;
		}
	}

	if (nchanges == 0)
		return 0;

	if (kevent(mux->kq_fd, changes, nchanges, NULL, 0, NULL) < 0) {
		x_eval_errno();
		return -1;
	}
	return 0;
}

static int kqueue_mux_add(x_sockmux *mux, x_sock fd, short flags)
{
	x_assert(mux != NULL);
	if (mux->n_events >= mux->max_events)
		kqueue_resize(mux, mux->max_events << 1);

	if (kqueue_apply(mux, fd, flags, 1) < 0)
		return -1;

	++mux->n_events;
	return 0;
}

static int kqueue_mux_mod(x_sockmux *mux, x_sock fd, short flags)
{
	x_assert(mux != NULL);
	return kqueue_apply(mux, fd, flags, 0);
}

static int kqueue_mux_del(x_sockmux *mux, x_sock fd)
{
	struct kevent changes[2];
	x_assert(mux != NULL);

	EV_SET(&changes[0], fd, EVFILT_READ, EV_DELETE, 0, 0, NULL);
	EV_SET(&changes[1], fd, EVFILT_WRITE, EV_DELETE, 0, 0, NULL);

	/* Deleting a non-existent filter returns ENOENT; ignore that error */
	kevent(mux->kq_fd, changes, 2, NULL, 0, NULL);

	mux->n_events--;
	mux->iterator = 0;
	return 0;
}

static int kqueue_mux_poll(x_sockmux *mux, struct timeval *timeout)
{
	int nreadys;
	struct timespec ts;
	struct timespec *tsp = NULL;

	x_assert(mux != NULL);
	mux->iterator = 0;
	mux->nreadys = 0;

	if (timeout) {
		ts.tv_sec = timeout->tv_sec;
		ts.tv_nsec = timeout->tv_usec * 1000;
		tsp = &ts;
	}

	nreadys = kevent(mux->kq_fd, NULL, 0, mux->events, mux->n_events, tsp);
	if (nreadys > 0)
		mux->nreadys = nreadys;
	else if (nreadys < 0) {
		x_eval_errno();
	}

	return nreadys;
}

static x_sock kqueue_mux_next(x_sockmux *mux, short *res_flags)
{
	*res_flags = 0;
	for (int i = mux->iterator; i < mux->nreadys; i++) {
		struct kevent *ev = &mux->events[i];

		if (ev->filter == EVFILT_READ) {
			*res_flags |= X_EV_READ;
		} else if (ev->filter == EVFILT_WRITE) {
			*res_flags |= X_EV_WRITE;
		}

		/* kqueue expresses errors via the EV_EOF / EV_ERROR flags */
		if (ev->flags & EV_EOF)
			*res_flags |= X_EV_ERROR;

		mux->iterator = i + 1;
		if (*res_flags)
			return (x_sock)ev->ident;
	}
	return X_BADSOCK;
}

const struct x_sockmux_ops_st g_sockmux_ops = {
	.m_add = kqueue_mux_add,
	.m_mod = kqueue_mux_mod,
	.m_del = kqueue_mux_del,
	.m_create = kqueue_mux_create,
	.m_free = kqueue_mux_free,
	.m_poll = kqueue_mux_poll,
	.m_next = kqueue_mux_next,
};

