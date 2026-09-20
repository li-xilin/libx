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
#include "x/ipcsock.h"
#include "x/path.h"
#include "x/detect.h"
#include "x/socket.h"
#include "x/errno.h"
#include "x/file.h"
#include "x/debug.h"

#define BACKLOG 256

#if defined(X_OS_WIN)
#define __UNIX_PATH_MAX 108
#define __AF_UNIX 1
struct __sockaddr_un {
	ADDRESS_FAMILY sun_family;
	char sun_path[__UNIX_PATH_MAX];
};
#else
#define __AF_UNIX AF_UNIX
#include <unistd.h>
#include <sys/un.h>
#endif

#if defined(X_OS_WIN)
static int win32_create_unix_file(const x_uchar *sock_file, uint16_t port)
{
	int retval = -1;
	FILE *fp = x_fopen(sock_file, x_u("w+"));
	if (!fp) {
		return -1;
	}
	uint16_t save_port = htons(port);
	if (fwrite(&save_port, sizeof save_port, 1, fp) < 1)
		goto out;
	retval = 0;
out:
	fclose(fp);
	return retval;
}

static int win32_read_unix_file(const x_uchar *sock_file, uint16_t *port)
{
	int retval = -1;
	FILE *fp = x_fopen(sock_file, x_u("r"));
	if (!fp) {
		return -1;
	}
	uint16_t save_port;
	if (fread(&save_port, sizeof save_port, 1, fp) < 1)
		goto out;
	if (fseek(fp, 0, SEEK_END))
		goto out;
	long end = ftell(fp);
	if (end == -1)
		goto out;
	if (end != sizeof(uint16_t)) {
		errno = X_EINVAL;
		goto out;
	}
	*port = ntohs(save_port);
	retval = 0;
out:
	fclose(fp);
	return retval;
}
#endif

x_sock x_ipcsock_listen(const x_uchar *sock_file, bool nonblock)
{
	x_sock listen_fd;
#if defined(X_OS_WIN)
	bool use_unix = true;
	char file_utf8[__UNIX_PATH_MAX + 2];
	if (x_ustr_to_utf8(sock_file, file_utf8, sizeof file_utf8) == sizeof file_utf8) {
		errno = X_EINVAL;
		return X_BADSOCK;
	}
	if (strlen(file_utf8) > __UNIX_PATH_MAX) {
		errno = X_ENAMETOOLONG;
		return X_BADSOCK;
	}
	listen_fd = socket(__AF_UNIX, SOCK_STREAM, 0);
	if (listen_fd != INVALID_SOCKET) {
		if (nonblock && x_sock_set_nonblocking(listen_fd, true)) {
			closesocket(listen_fd);
			return X_BADSOCK;
		}
		struct __sockaddr_un sa = { .sun_family = __AF_UNIX };
		strncpy(sa.sun_path, file_utf8, sizeof sa.sun_path);
		if (bind(listen_fd, (struct sockaddr *)&sa, sizeof sa)) {
			x_eval_errno();
			closesocket(listen_fd);
			return X_BADSOCK;
		}
		if (listen(listen_fd, BACKLOG)) {
			x_eval_errno();
			closesocket(listen_fd);
			return X_BADSOCK;
		}
		return listen_fd;
	}
	listen_fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (listen_fd == INVALID_SOCKET) {
		x_eval_errno();
		return X_BADSOCK;
	}
	if (nonblock && x_sock_set_nonblocking(listen_fd, true)) {
		closesocket(listen_fd);
		return X_BADSOCK;
	}
	struct sockaddr_in sa = {
		.sin_family = AF_INET,
		.sin_addr.S_un.S_un_b = { 127, 0, 0, 1 },
		.sin_port = 0,
	};
	if (bind(listen_fd, (struct sockaddr *)&sa, sizeof sa)) {
		x_eval_errno();
		closesocket(listen_fd);
		return X_BADSOCK;
	}
	if (listen(listen_fd, BACKLOG)) {
		closesocket(listen_fd);
		return X_BADSOCK;
	}
	int addr_len = sizeof sa;
	if (getsockname(listen_fd, (struct sockaddr *)&sa, &addr_len) == SOCKET_ERROR) {
		x_eval_errno();
		x_eval_errno();
		return X_BADSOCK;
	}
	if (win32_create_unix_file(sock_file, ntohs(sa.sin_port))) {
		closesocket(listen_fd);
		return X_BADSOCK;
	}
	return listen_fd;
#else
	if (strlen(sock_file) > 108) {
		errno = X_ENAMETOOLONG;
		return X_BADSOCK;
	}
	listen_fd = socket(__AF_UNIX, SOCK_STREAM, 0);
	if (listen_fd == -1)
		return X_BADSOCK;
	if (nonblock && x_sock_set_nonblocking(listen_fd, true)) {
		close(listen_fd);
		return X_BADSOCK;
	}
	struct sockaddr_un sa = { .sun_family = AF_UNIX };
	strcpy(sa.sun_path, sock_file);
	if (bind(listen_fd, (struct sockaddr *)&sa, sizeof sa)) {
		x_eval_errno();
		close(listen_fd);
		return X_BADSOCK;
	}
	if (listen(listen_fd, BACKLOG)) {
		x_eval_errno();
		close(listen_fd);
		return X_BADSOCK;
	}

	return listen_fd;
#endif
}

x_sock x_ipcsock_connect(const x_uchar *sock_file, bool nonblock)
{
	x_sock client_fd;
#if defined(X_OS_WIN)
	bool use_unix = true;
	char file_utf8[__UNIX_PATH_MAX + 2];
	if (x_ustr_to_utf8(sock_file, file_utf8, sizeof file_utf8) == sizeof file_utf8) {
		errno = X_EINVAL;
		return X_BADSOCK;
	}
	if (strlen(file_utf8) > __UNIX_PATH_MAX) {
		errno = X_ENAMETOOLONG;
		return X_BADSOCK;
	}
	client_fd = socket(__AF_UNIX, SOCK_STREAM, 0);
	if (client_fd != INVALID_SOCKET) {
		if (nonblock && x_sock_set_nonblocking(client_fd, true)) {
			closesocket(client_fd);
			return X_BADSOCK;
		}
		struct __sockaddr_un sa = { .sun_family = __AF_UNIX };
		strcpy(sa.sun_path, file_utf8);
		if (connect(client_fd, (struct sockaddr *)&sa, sizeof sa)) {
			x_eval_errno();
			closesocket(client_fd);
			return X_BADSOCK;
		}
		return client_fd;
	}
	client_fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (client_fd == INVALID_SOCKET) {
		x_eval_errno();
		return X_BADSOCK;
	}
	if (nonblock && x_sock_set_nonblocking(client_fd, true)) {
		closesocket(client_fd);
		return X_BADSOCK;
	}
	uint16_t port;
	if (win32_read_unix_file(sock_file, &port)) {
		x_eval_errno();
		closesocket(client_fd);
		return X_BADSOCK;
	}
	struct sockaddr_in sa = {
		.sin_family = AF_INET,
		.sin_addr.S_un.S_un_b = { 127, 0, 0, 1 },
		.sin_port = htons(port),
	};
	if (connect(client_fd, (struct sockaddr *)&sa, sizeof sa)) {
		x_eval_errno();
		closesocket(client_fd);
		return X_BADSOCK;
	}
	return client_fd;
#else
	if (strlen(sock_file) > 108) {
		errno = X_ENAMETOOLONG;
		return X_BADSOCK;
	}
	client_fd = socket(__AF_UNIX, SOCK_STREAM, 0);
	if (client_fd == -1) {
		x_eval_errno();
		return X_BADSOCK;
	}
	if (nonblock && x_sock_set_nonblocking(client_fd, true)) {
		x_eval_errno();
		close(client_fd);
		return X_BADSOCK;
	}
	struct sockaddr_un sa = { .sun_family = AF_UNIX };
	strcpy(sa.sun_path, sock_file);
	if (connect(client_fd, (struct sockaddr *)&sa, sizeof sa)) {
		x_eval_errno();
		close(client_fd);
		return X_BADSOCK;
	}
	return client_fd;
#endif
	return 0;
}

int x_ipcsock_set_fastpath(x_sock fd, bool enable)
{
#if defined(X_OS_WIN)
	DWORD opt = enable;
	DWORD bytesReturned = 0;
	int ret = WSAIoctl(fd, SIO_LOOPBACK_FAST_PATH, &opt,
			sizeof(opt), NULL, 0, &bytesReturned, NULL, NULL);
	if (ret == SOCKET_ERROR) {
		x_eval_errno();
		return -1;
	}
#endif
	return 0;
}

