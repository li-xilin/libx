/*
 * Copyright (c) 2023,2025 Li Xilin <lixilin@gmx.com>
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

#ifndef X_ERROR_H
#define X_ERROR_H

#include "x/detect.h"
#include "x/log.h"
#include "x/errno.h"
#include <stddef.h>
#include <errno.h>

#define __X_EBASE 700

#define X_E2BIG         E2BIG
#define X_EACCES        EACCES
#define X_EAGAIN        EAGAIN
#define X_EBADF         EBADF
#define X_EBUSY         EBUSY
#define X_ECHILD        ECHILD
#define X_EDEADLK       EDEADLK
#define X_EDEADLOCK     EDEADLOCK
#define X_EDOM          EDOM
#define X_EEXIST        EEXIST
#define X_EFAULT        EFAULT
#define X_EFBIG         EFBIG
#define X_EILSEQ        EILSEQ
#define X_EINTR         EINTR
#define X_EINVAL        EINVAL
#define X_EIO           EIO
#define X_EISDIR        EISDIR
#define X_EMFILE        EMFILE
#define X_EMLINK        EMLINK
#define X_ENAMETOOLONG  ENAMETOOLONG
#define X_ENFILE        ENFILE
#define X_ENODEV        ENODEV
#define X_ENOENT        ENOENT
#define X_ENOEXEC       ENOEXEC
#define X_ENOLCK        ENOLCK
#define X_ENOMEM        ENOMEM
#define X_ENOSPC        ENOSPC
#define X_ENOSYS        ENOSYS
#define X_ENOTDIR       ENOTDIR
#define X_ENOTEMPTY     ENOTEMPTY
#define X_ENOTTY        ENOTTY
#define X_ENXIO         ENXIO
#define X_EPERM         EPERM
#define X_EPIPE         EPIPE
#define X_ERANGE        ERANGE
#define X_EROFS         EROFS
#define X_ESPIPE        ESPIPE
#define X_ESRCH         ESRCH
#define X_EXDEV         EXDEV
#define X_EADDRINUSE    EADDRINUSE
#define X_EADDRNOTAVAIL EADDRNOTAVAIL
#define X_EAFNOSUPPORT  EAFNOSUPPORT
#define X_EALREADY      EALREADY
#define X_EBADMSG       EBADMSG
#define X_ECANCELED     ECANCELED
#define X_ECONNABORTED  ECONNABORTED
#define X_ECONNREFUSED  ECONNREFUSED
#define X_ECONNRESET    ECONNRESET
#define X_EDESTADDRREQ  EDESTADDRREQ
#define X_EHOSTUNREACH  EHOSTUNREACH
#define X_EIDRM         EIDRM
#define X_EINPROGRESS   EINPROGRESS
#define X_EISCONN       EISCONN
#define X_ELOOP         ELOOP
#define X_EMSGSIZE      EMSGSIZE
#define X_ENETDOWN      ENETDOWN
#define X_ENETRESET     ENETRESET
#define X_ENETUNREACH   ENETUNREACH
#define X_ENOBUFS       ENOBUFS
#define X_ENODATA       ENODATA
#define X_ENOLINK       ENOLINK
#define X_ENOMSG        ENOMSG
#define X_ENOPROTOOPT   ENOPROTOOPT
#define X_ENOSR         ENOSR
#define X_ENOSTR        ENOSTR
#define X_ENOTCONN      ENOTCONN
#define X_ENOTRECOVERABLE    ENOTRECOVERABLE
#define X_ENOTSOCK      ENOTSOCK
#define X_ENOTSUP       ENOTSUP
#define X_EOPNOTSUPP    EOPNOTSUPP
#define X_EOVERFLOW     EOVERFLOW
#define X_EOWNERDEAD    EOWNERDEAD
#define X_EPROTO        EPROTO
#define X_EPROTONOSUPPORT    EPROTONOSUPPORT
#define X_EPROTOTYPE    EPROTOTYPE
#define X_ETIME         ETIME
#define X_ETIMEDOUT     ETIMEDOUT
#define X_ETXTBSY       ETXTBSY
#define X_EWOULDBLOCK   EWOULDBLOCK

#define X_EDQUOT __X_EBASE + 1

int x_eval_errno(void);

#endif

