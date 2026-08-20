/*
 * Copyright (c) 2023 Li Xilin <lixilin@gmx.com>
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

#include "x/types.h"
#include "x/thread.h"
#include "x/tsignal.h"
#include "x/detect.h"
#include "x/errno.h"

static x_tsignal_fn *s_signal_handler = NULL;

#ifdef X_OS_WIN
#include <windows.h>
static BOOL WINAPI CtrlHandler(DWORD fdwCtrlType)
{
	switch( fdwCtrlType) {
		case CTRL_C_EVENT:
			s_signal_handler(X_TSIGNAL_INT);
			return TRUE;
		case CTRL_BREAK_EVENT:
			s_signal_handler(X_TSIGNAL_QUIT);
			return TRUE;
		case CTRL_CLOSE_EVENT:
		case CTRL_LOGOFF_EVENT:
		case CTRL_SHUTDOWN_EVENT:
			s_signal_handler(X_TSIGNAL_CLOSE);
			return TRUE;
		default:
			return FALSE;
	}
}

int x_tsignal_set(x_tsignal_fn *proc)
{
	if (!proc) {
		errno = X_EINVAL;
		return -1;
	}
	if (s_signal_handler == NULL) {
		if(!SetConsoleCtrlHandler((PHANDLER_ROUTINE) CtrlHandler,TRUE)) {
			x_eval_errno();
			return -1;
		}
	}
	s_signal_handler = proc;
	return 0;
}

void x_tsignal_reset(void)
{
	if (!s_signal_handler)
		return;
	SetConsoleCtrlHandler((PHANDLER_ROUTINE) CtrlHandler,FALSE);
	s_signal_handler = NULL;
}

#else
#include <stdlib.h>
#include <signal.h>
#include <pthread.h>
#include <unistd.h>

struct sigaction sg_sigint_sa;
struct sigaction sg_sigterm_sa;
struct sigaction sg_sigquit_sa;

static void *thread_proc(void *arg)
{
	int action = *(int *)arg;
	free(arg);
	s_signal_handler(action);
	return NULL;
}

static void signal_handler (int sig)
{
	int *act = malloc(sizeof(int));
	if (!act)
		return;
	switch (sig) {
		case SIGINT:
			*act = X_TSIGNAL_INT;
			break;
		case SIGQUIT:
			*act = X_TSIGNAL_QUIT;
			break;
		case SIGTERM:
			*act = X_TSIGNAL_CLOSE;
			break;
	}
	pthread_t thd;
	if (pthread_create(&thd, NULL, thread_proc, act))
		return;
	if (sig == SIGTERM) {
		pthread_join(thd, 0);
		_exit(143);
	}
	pthread_detach(thd);
}

int x_tsignal_set(x_tsignal_fn *proc)
{
	if (!proc) {
		errno = X_EINVAL;
		return -1;
	}
	if (s_signal_handler == NULL) {
		struct sigaction sa = {
			.sa_handler = signal_handler,
			.sa_flags = 0
#ifdef SA_NODEFER
				| SA_NODEFER
#endif
#ifdef SA_RESTART
				| SA_RESTART
#endif
		};
		if (sigaction(SIGINT, &sa, &sg_sigint_sa))
			goto fail;
		if (sigaction(SIGQUIT, &sa, &sg_sigquit_sa))
			goto fail;
		if (sigaction(SIGTERM, &sa, &sg_sigterm_sa))
			goto fail;
	}
	s_signal_handler = proc;
	return 0;
fail:
	x_eval_errno();
	return -1;
}

void x_tsignal_reset(void)
{
	if (!s_signal_handler)
		return;
	(void)sigaction(SIGINT, &sg_sigint_sa, NULL);
	(void)sigaction(SIGQUIT, &sg_sigquit_sa, NULL);
	(void)sigaction(SIGTERM, &sg_sigterm_sa, NULL);
	s_signal_handler = NULL;
}

#endif

int nomem_thread_proc(void)
{
	x_tsignal_fn *handler = x_thread_data();
	handler(X_TSIGNAL_NOMEM);
	abort();
	return 0;
}

void x_tsignal_send_nomem(void)
{
	x_tsignal_fn *handler = s_signal_handler;
	if (!handler) {
		abort();
		return;
	}
	x_thread *t = x_thread_create(nomem_thread_proc, NULL, handler);
	if (t)
		x_thread_free(t);
	while (true)
		x_thread_sleep(1000);
}

