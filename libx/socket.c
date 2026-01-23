/*
 * Copyright (c) 2022-2023 Li Xilin <lixilin@gmx.com>
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

#include "x/socket.h"
#include "x/log.h"
#include "x/string.h"
#include "x/trick.h"
#include "x/detect.h"
#include "x/errno.h"

#ifdef X_OS_WIN32
#include <ws2tcpip.h>
#include <winsock.h>
#include <windows.h>

#ifdef HAVE_AFUNIX_H
#include <afunix.h>
#endif

#else
#include <unistd.h>
#include <fcntl.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <netinet/tcp.h>
#endif

#include <errno.h>
#include <string.h>
#include <assert.h>
#include <stdbool.h>

#ifdef HAVE_AFUNIX_H
int have_working_afunix_ = -1;
#endif

int x_sock_init(void)
{
#ifdef X_OS_WIN32
        WSADATA wsaData;
        WORD wVersionRequested = MAKEWORD(1, 1);
        return WSAStartup(wVersionRequested, &wsaData);
#else
	return 0;
#endif
}

void x_sock_exit(void)
{
#ifdef X_OS_WIN32
	WSACleanup();
#endif
}

int x_sock_close(x_sock fd) {
	if (fd == X_BADSOCK)
		return 0;
#ifdef X_OS_WIN32
	return - !closesocket(fd);
#else
	return close(fd);
#endif
}


int x_sock_set_nonblocking(x_sock fd)
{
#ifdef X_OS_WIN32
	ULONG nonBlock = 1;
	if (ioctlsocket(fd, FIONBIO, &nonBlock)) {
		x_perror("failed on ioctlsocket");
		return (-1);
	}
	return (0);
#else
	int opts;

	if ((opts = fcntl(fd, F_GETFL)) < 0) {
		x_perror("failed on fcntl before: %s", strerror(errno));
		return (-1);
	}

	opts |= O_NONBLOCK;
	if (fcntl(fd, F_SETFL, opts) < 0) {
		x_perror("failed on fcntl after: %s", strerror(errno));
		return (-1);
	}

	return (0);
#endif
}

#ifdef X_OS_WIN32

static int socketpair_ersatz(int family, int type, int protocol, x_sock fd[2])
{
	assert(fd);

	x_sock listener = -1;
	x_sock connector = -1;
	x_sock acceptor = -1;
	struct sockaddr_in listen_addr;
	struct sockaddr_in connect_addr;
	socklen_t size;
	int saved_errno = -1;
	int family_test;
	
	family_test = family != AF_INET;
#ifdef AF_UNIX
	family_test = family_test && (family != AF_UNIX);
#endif
	if (protocol || family_test) {
		x_perror("family is not supported");
		return -1;
	}
	
	listener = socket(AF_INET, type, 0);
	if (listener < 0)
		return -1;
	memset(&listen_addr, 0, sizeof(listen_addr));
	listen_addr.sin_family = AF_INET;
	listen_addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
	listen_addr.sin_port = 0;	/* kernel chooses port.	 */
	if (bind(listener, (struct sockaddr *) &listen_addr, sizeof (listen_addr)) == -1)
		goto tidy_up_and_fail;
	if (listen(listener, 1) == -1)
		goto tidy_up_and_fail;

	connector = socket(AF_INET, type, 0);
	if (connector < 0)
		goto tidy_up_and_fail;

	memset(&connect_addr, 0, sizeof(connect_addr));

	/* We want to find out the port number to connect to.  */
	size = sizeof(connect_addr);
	if (getsockname(listener, (struct sockaddr *) &connect_addr, &size) == -1)
		goto tidy_up_and_fail;
	if (size != sizeof (connect_addr))
		goto abort_tidy_up_and_fail;
	if (connect(connector, (struct sockaddr *) &connect_addr,
				sizeof(connect_addr)) == -1)
		goto tidy_up_and_fail;

	size = sizeof(listen_addr);
	acceptor = accept(listener, (struct sockaddr *) &listen_addr, &size);
	if (acceptor < 0)
		goto tidy_up_and_fail;
	if (size != sizeof(listen_addr))
		goto abort_tidy_up_and_fail;
	/* Now check we are talking to ourself by matching port and host on the
	   two sockets.	 */
	if (getsockname(connector, (struct sockaddr *) &connect_addr, &size) == -1)
		goto tidy_up_and_fail;
	if (size != sizeof (connect_addr)
		|| listen_addr.sin_family != connect_addr.sin_family
		|| listen_addr.sin_addr.s_addr != connect_addr.sin_addr.s_addr
		|| listen_addr.sin_port != connect_addr.sin_port)
		goto abort_tidy_up_and_fail;
	x_sock_close(listener);
	fd[0] = connector;
	fd[1] = acceptor;

	return 0;

 abort_tidy_up_and_fail:
	saved_errno = WSAECONNABORTED;
 tidy_up_and_fail:
	if (saved_errno < 0)
		saved_errno = WSAGetLastError();
	if (listener != -1)
		x_sock_close(listener);
	if (connector != -1)
		x_sock_close(connector);
	if (acceptor != -1)
		x_sock_close(acceptor);

	WSASetLastError(saved_errno);
	return -1;
}

#ifdef HAVE_AFUNIX_H
static int create_tmpfile(char tmpfile[MAX_PATH])
{
	char short_path[MAX_PATH] = {0};
	char long_path[MAX_PATH] = {0};
	char prefix[4] = "axe";

	GetTempPathA(MAX_PATH, short_path);
	GetLongPathNameA(short_path, long_path, MAX_PATH);
	if (!GetTempFileNameA(long_path, prefix, 0, tmpfile)) {
		x_pwarn("GetTempFileName failed: %d", GetLastError());
		return -1;
	}
	return 0;
}

/* Test whether Unix domain socket works.
 * Return 1 if it works, otherwise 0 */
static int check_working_afunix()
{
	/* Windows 10 began to support Unix domain socket. Let's just try
	 * socket(AF_UNIX, , ) and fall back to socket(AF_INET, , ).
	 * https://devblogs.microsoft.com/commandline/af_unix-comes-to-windows/
	 */
	x_sock sd = -1;
	if (have_working_afunix_ < 0) {
		if ((sd = socket(AF_UNIX, SOCK_STREAM, 0)) < 0) {
			have_working_afunix_ = 0;
		}
		else {
			have_working_afunix_ = 1;
			x_sock_close(sd);
		}
	}
	return have_working_afunix_;
}

static int socketpair_win32_afunix(int family, int type, int protocol, x_sock fd[2])
{
	assert(fd);

	x_sock listener = -1;
	x_sock connector = -1;
	x_sock acceptor = -1;

	struct sockaddr_un listen_addr;
	struct sockaddr_un connect_addr;
	char tmp_file[MAX_PATH] = {0};

	socklen_t size;
	int saved_errno = -1;

	listener = socket(family, type, 0);
	if (listener < 0)
		return -1;

	memset(&listen_addr, 0, sizeof(listen_addr));

	if (create_tmpfile(tmp_file)) {
		goto tidy_up_and_fail;
	}
	DeleteFileA(tmp_file);
	listen_addr.sun_family = AF_UNIX;
	if (strlen(tmp_file) >= UNIX_PATH_MAX) {
		x_pwarn("Temp file name is too long");
		goto tidy_up_and_fail;
	}
	strcpy(listen_addr.sun_path, tmp_file);

	if (bind(listener, (struct sockaddr *) &listen_addr, sizeof (listen_addr))
		== -1)
		goto tidy_up_and_fail;
	if (listen(listener, 1) == -1)
		goto tidy_up_and_fail;

	connector = socket(family, type, 0);
	if (connector < 0)
		goto tidy_up_and_fail;

	memset(&connect_addr, 0, sizeof(connect_addr));

	/* We want to find out the port number to connect to.  */
	size = sizeof(connect_addr);
	if (getsockname(listener, (struct sockaddr *) &connect_addr, &size) == -1)
		goto tidy_up_and_fail;

	if (connect(connector, (struct sockaddr *) &connect_addr,
				sizeof(connect_addr)) == -1)
		goto tidy_up_and_fail;

	size = sizeof(listen_addr);
	acceptor = accept(listener, (struct sockaddr *) &listen_addr, &size);
	if (acceptor < 0)
		goto tidy_up_and_fail;
	if (size != sizeof(listen_addr))
		goto abort_tidy_up_and_fail;
	/* Now check we are talking to ourself by matching port and host on the
	   two sockets.	 */
	if (getsockname(connector, (struct sockaddr *) &connect_addr, &size) == -1)
		goto tidy_up_and_fail;

	if (size != sizeof(connect_addr) ||
	    listen_addr.sun_family != connect_addr.sun_family || _stricmp(listen_addr.sun_path, connect_addr.sun_path))
		goto abort_tidy_up_and_fail;

	x_sock_close(listener);
	fd[0] = connector;
	fd[1] = acceptor;

	return 0;

 abort_tidy_up_and_fail:
	saved_errno = X_SOCKET_ERR(ECONNABORTED);
 tidy_up_and_fail:
	if (saved_errno < 0)
		saved_errno = x_sock_errno();
	if (listener != -1)
		x_sock_close(listener);
	if (connector != -1)
		x_sock_close(connector);
	if (acceptor != -1)
		x_sock_close(acceptor);
	if (tmp_file[0])
		DeleteFileA(tmp_file);

	x_sock_set_errno(saved_errno);
	return -1;
}
#endif

static int socketpair_win32(int family, int type, int protocol, x_sock fd[2])
{
#ifdef HAVE_AFUNIX_H
	if (protocol || (family != AF_UNIX && family != AF_INET))
		return -1;

	if (check_working_afunix()) {
		family = AF_UNIX;
		if (type != SOCK_STREAM) {
			/* Win10 does not support AF_UNIX socket of a type other
			 * than SOCK_STREAM still now. */
			family = AF_INET;
		}
	} else {
		family = AF_INET;
	}
	if (family == AF_UNIX)
		return socketpair_win32_afunix(family, type, protocol, fd);

#endif
	return socketpair_ersatz(family, type, protocol, fd);
}
#endif

int x_sock_pair(int family, int type, int protocol, x_sock fd[2])
{
#ifdef X_OS_WIN32
	return socketpair_win32(family, type, protocol, fd);
#else
	return socketpair(family, type, protocol, fd);
#endif
}

int x_sock_sendall(x_sock sock, const void *data, size_t len)
{
	size_t sent = 0;
	int flags = 0;
#ifndef X_OS_WIN32
	flags = MSG_NOSIGNAL;
#endif
	while (sent != len) {
		ssize_t ret = send(sock, (char *)data + sent, len - sent, flags);
		if (ret <  0) {
#ifndef X_OS_WIN32
			if (errno == X_EINTR)
				continue;
#endif
			return -1;
		}
		sent += ret;

	}
	return 0;
}

int x_sock_recvall(x_sock sock, void *buf, size_t len)
{
	if (len == 0)
		return 0;
	size_t received = 0;
	while (received != len) {
		ssize_t ret = recv(sock, (char *)buf + received, len - received, MSG_WAITALL);
		if (ret <  0) {
#ifndef X_OS_WIN32
			if (errno == X_EINTR) {
				continue;
			}
#endif
			return -1;
		}
		if (ret == 0) {
			errno = X_ENODATA;
			return -1;
		}
		received += ret;

	}
	return 0;
}

static int wait_socket(const x_sock *sock, size_t cnt, bool readable, size_t millise)
{
	int nreadys;
	x_sock fd_max = 0;
	fd_set fdset;
	FD_ZERO(&fdset);
	for (int i = 0; i < cnt; i++) {
		if (sock[i] > fd_max)
			fd_max = sock[i];
		FD_SET(sock[i], &fdset);
	}
	struct timeval timeout = {
		.tv_sec = millise / 1000,
		.tv_usec = millise % 1000 * 1000,
	};
redo:
	 nreadys = readable
		 ? select(fd_max + 1, &fdset, NULL, NULL, &timeout)
		 : select(fd_max + 1, NULL, &fdset, NULL, &timeout);
	 if (nreadys == 0) {
		 errno = X_ETIMEDOUT;
		 return -1;
	 }
	 if (nreadys < 0) {
#ifndef X_OS_WIN32
		 if (errno == X_EINTR)
			 goto redo;
#endif
		 return -1;
	 }
	 for (int i = 0; i < cnt; i++) {
		 if (FD_ISSET(sock[i], &fdset))
			 return i;
	 }
	 goto redo;
	 return -1;
}

int x_sock_wait_writable(const x_sock *sock, size_t cnt, size_t millise)
{
	return wait_socket(sock, cnt, false, millise);
}

int x_sock_wait_readable(const x_sock *sock, size_t cnt, size_t millise)
{
	return wait_socket(sock, cnt, true, millise);
}

int x_sock_set_keepalive(x_sock sock, uint32_t idle_sec, uint32_t interval_sec)
{
	int old_keep_alive;
	socklen_t size = sizeof old_keep_alive;
	if (getsockopt(sock, SOL_SOCKET, SO_KEEPALIVE, (char *)&old_keep_alive, &size))
		return -1;
	if (!old_keep_alive && setsockopt(sock, SOL_SOCKET, SO_KEEPALIVE, (char *)x_p(int, 1), sizeof(int)))
		return -1;

#ifdef X_OS_WIN32
	struct tcp_keepalive keepalive = {
		.onoff = 1,
		.keepalivetime = idle_sec * 1000,
		.keepaliveinterval = interval_sec * 1000,
	};
	if (WSAIoctl(sock, SIO_KEEPALIVE_VALS, &keepalive, sizeof(keepalive), NULL, 0, NULL, NULL, NULL) == SOCKET_ERROR)
		goto fail;
#else
	if (setsockopt(sock, IPPROTO_TCP, TCP_KEEPIDLE, &idle_sec, sizeof idle_sec))
		goto fail;

	if (setsockopt(sock, IPPROTO_TCP, TCP_KEEPINTVL, &interval_sec, sizeof interval_sec))
		goto fail;

	//Campatible with NT6.0 or later.
	if (setsockopt(sock, IPPROTO_TCP, TCP_KEEPCNT, (char *)x_p(int, 10), sizeof(int)))
		goto fail;
#endif

	return 0;
fail:
	(void)setsockopt(sock, SOL_SOCKET, SO_KEEPALIVE, (char *)&old_keep_alive, sizeof old_keep_alive);
	return -1;
}
