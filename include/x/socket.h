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

#ifndef X_SOCKET_H
#define	X_SOCKET_H
#include "log.h"
#include "detect.h"
#include <stddef.h>

#ifdef X_OS_WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
typedef SOCKET x_sock;
#define X_BADSOCK INVALID_SOCKET
#ifndef MSG_NOSIGNAL
#define MSG_NOSIGNAL 0
#endif
#else
#include<sys/socket.h>
#include<sys/types.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#include<endian.h>
typedef int x_sock;
#define X_BADSOCK -1
#endif

#ifdef X_OS_WIN32
#define x_sock_errno() WSAGetLastError()
#define x_sock_set_errno(errcode) WSASetLastError(errcode)
#define X_SOCK_ERR(name) WSA##name
#else
#define x_sock_errno() errno
#define x_sock_set_errno(errcode) (void)(errno = (errcode))
#define X_SOCK_ERR(name) name
#endif

struct timeval;

int x_sock_init(void);

void x_sock_exit(void);

int x_sock_pair(int family, int type, int protocol, x_sock fd[2]);

int x_sock_close(x_sock fd);

int x_sock_set_nonblocking(x_sock fd);

int x_sock_sendall(x_sock sock, const void *data, size_t len);

int x_sock_recvall(x_sock sock, void *buf, size_t len);

int x_sock_wait_readable(x_sock sock, size_t millise);

int x_sock_wait_writable(x_sock sock, size_t millise);

inline static int x_sock_recv_u16(x_sock sock, uint16_t *valuep)
{
        uint16_t value;
        if (x_sock_recvall(sock, &value, sizeof value))
                return -1;
        *valuep = ntohs(value);
        return 0;
}

inline static int x_sock_recv_u32(x_sock sock, uint32_t *valuep)
{
        uint32_t value;
        if (x_sock_recvall(sock, &value, sizeof value))
                return -1;
        *valuep = ntohl(value);
        return 0;
}

inline static int x_sock_send_u16(x_sock sock, uint16_t value)
{
        uint16_t value_n = htons(value);
        if (x_sock_sendall(sock, &value_n, sizeof value_n))
                return -1;
        return 0;
}

inline static int x_sock_send_u32(x_sock sock, uint32_t value)
{
        uint32_t value_n = htonl(value);
        if (x_sock_sendall(sock, &value_n, sizeof value_n))
                return -1;
        return 0;
}

int x_sock_set_keepalive(x_sock sock, uint32_t idle_sec, uint32_t interval_sec);

#endif
