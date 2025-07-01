/*
 * Copyright (c) 2023 Li Xilin <lixilin@gmx.com>
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

#include "x/errno.h"
#include <x/detect.h>
#include <stdlib.h>

#ifdef X_OS_WIN
#include <errhandlingapi.h>
#include <windows.h>
#endif

#ifndef X_OS_WIN
static int errno_is_common(int err)
{
	switch (err) {
		case X_E2BIG:
		case X_EACCES:
		case X_EAGAIN:
		case X_EBADF:
		case X_EBUSY:
		case X_ECHILD:
		case X_EDEADLK:
		case X_EDOM:
		case X_EEXIST:
		case X_EFAULT:
		case X_EFBIG:
		case X_EILSEQ:
		case X_EINTR:
		case X_EINVAL:
		case X_EIO:
		case X_EISDIR:
		case X_EMFILE:
		case X_EMLINK:
		case X_ENAMETOOLONG:
		case X_ENFILE:
		case X_ENODEV:
		case X_ENOENT:
		case X_ENOEXEC:
		case X_ENOLCK:
		case X_ENOMEM:
		case X_ENOSPC:
		case X_ENOSYS:
		case X_ENOTDIR:
		case X_ENOTEMPTY:
		case X_ENOTTY:
		case X_ENXIO:
		case X_EPERM:
		case X_EPIPE:
		case X_ERANGE:
		case X_EROFS:
		case X_ESPIPE:
		case X_ESRCH:
		case X_EXDEV:
		case X_EADDRINUSE:
		case X_EADDRNOTAVAIL:
		case X_EAFNOSUPPORT:
		case X_EALREADY:
		case X_EBADMSG:
		case X_ECANCELED:
		case X_ECONNABORTED:
		case X_ECONNREFUSED:
		case X_ECONNRESET:
		case X_EDESTADDRREQ:
		case X_EHOSTUNREACH:
		case X_EIDRM:
		case X_EINPROGRESS:
		case X_EISCONN:
		case X_ELOOP:
		case X_EMSGSIZE:
		case X_ENETDOWN:
		case X_ENETRESET:
		case X_ENETUNREACH:
		case X_ENOBUFS:
		case X_ENODATA:
		case X_ENOLINK:
		case X_ENOMSG:
		case X_ENOPROTOOPT:
		case X_ENOSR:
		case X_ENOSTR:
		case X_ENOTCONN:
		case X_ENOTRECOVERABLE:
		case X_ENOTSOCK:
		case X_ENOTSUP:
		case X_EOVERFLOW:
		case X_EOWNERDEAD:
		case X_EPROTO:
		case X_EPROTONOSUPPORT:
		case X_EPROTOTYPE:
		case X_ETIME:
		case X_ETIMEDOUT:
		case X_ETXTBSY:
			return 1;
		default:
			return 0;
	}
}
#endif

#ifdef X_OS_WIN
static int eval_errno(int winerror)
{
	// Unwrap FACILITY_WIN32 HRESULT errors.
	if ((winerror & 0xFFFF0000) == 0x80070000) {
		winerror &= 0x0000FFFF;
	}

	// Winsock error codes (10000-11999) are errno values.
	if (winerror >= 10000 && winerror < 12000) {
		switch (winerror) {
			case WSAEINTR:
			case WSAEBADF:
			case WSAEACCES:
			case WSAEFAULT:
			case WSAEINVAL:
			case WSAEMFILE:
				// Winsock definitions of errno values. See WinSock2.h
				return winerror - 10000;
			default:
				return winerror;
		}
	}

	switch (winerror) {
		case ERROR_FILE_NOT_FOUND:
		case ERROR_PATH_NOT_FOUND:
		case ERROR_INVALID_DRIVE:
		case ERROR_NO_MORE_FILES:
		case ERROR_BAD_NETPATH:
		case ERROR_BAD_NET_NAME:
		case ERROR_BAD_PATHNAME:
		case ERROR_FILENAME_EXCED_RANGE:
			return X_ENOENT;

		case ERROR_BAD_ENVIRONMENT:
			return X_E2BIG;

		case ERROR_BAD_FORMAT:
		case ERROR_INVALID_STARTING_CODESEG:
		case ERROR_INVALID_STACKSEG:
		case ERROR_INVALID_MODULETYPE:
		case ERROR_INVALID_EXE_SIGNATURE:
		case ERROR_EXE_MARKED_INVALID:
		case ERROR_BAD_EXE_FORMAT:
		case ERROR_ITERATED_DATA_EXCEEDS_64k:
		case ERROR_INVALID_MINALLOCSIZE:
		case ERROR_DYNLINK_FROM_INVALID_RING:
		case ERROR_IOPL_NOT_ENABLED:
		case ERROR_INVALID_SEGDPL:
		case ERROR_AUTODATASEG_EXCEEDS_64k:
		case ERROR_RING2SEG_MUST_BE_MOVABLE:
		case ERROR_RELOC_CHAIN_XEEDS_SEGLIM:
		case ERROR_INFLOOP_IN_RELOC_CHAIN:
			return X_ENOEXEC;

		case ERROR_INVALID_HANDLE:
		case ERROR_INVALID_TARGET_HANDLE:
		case ERROR_DIRECT_ACCESS_HANDLE:
			return X_EBADF;

		case ERROR_WAIT_NO_CHILDREN:
		case ERROR_CHILD_NOT_COMPLETE:
			return X_ECHILD;

		case ERROR_NO_PROC_SLOTS:
		case ERROR_MAX_THRDS_REACHED:
		case ERROR_NESTING_NOT_ALLOWED:
			return X_EAGAIN;

		case ERROR_ARENA_TRASHED:
		case ERROR_NOT_ENOUGH_MEMORY:
		case ERROR_INVALID_BLOCK:
		case ERROR_NOT_ENOUGH_QUOTA:
			return X_ENOMEM;

		case ERROR_ACCESS_DENIED:
		case ERROR_CURRENT_DIRECTORY:
		case ERROR_WRITE_PROTECT:
		case ERROR_BAD_UNIT:
		case ERROR_NOT_READY:
		case ERROR_BAD_COMMAND:
		case ERROR_CRC:
		case ERROR_BAD_LENGTH:
		case ERROR_SEEK:
		case ERROR_NOT_DOS_DISK:
		case ERROR_SECTOR_NOT_FOUND:
		case ERROR_OUT_OF_PAPER:
		case ERROR_WRITE_FAULT:
		case ERROR_READ_FAULT:
		case ERROR_GEN_FAILURE:
		case ERROR_SHARING_VIOLATION:
		case ERROR_LOCK_VIOLATION:
		case ERROR_WRONG_DISK:
		case ERROR_SHARING_BUFFER_EXCEEDED:
		case ERROR_NETWORK_ACCESS_DENIED:
		case ERROR_CANNOT_MAKE:
		case ERROR_FAIL_I24:
		case ERROR_DRIVE_LOCKED:
		case ERROR_SEEK_ON_DEVICE:
		case ERROR_NOT_LOCKED:
		case ERROR_LOCK_FAILED:
		case 35:
			return X_EACCES;

		case ERROR_FILE_EXISTS:
		case ERROR_ALREADY_EXISTS:
			return X_EEXIST;

		case ERROR_NOT_SAME_DEVICE:
			return X_EXDEV;

		case ERROR_DIRECTORY:
			return X_ENOTDIR;

		case ERROR_TOO_MANY_OPEN_FILES:
			return X_EMFILE;

		case ERROR_DISK_FULL:
			return X_ENOSPC;

		case ERROR_BROKEN_PIPE:
		case ERROR_NO_DATA:
			return X_EPIPE;

		case ERROR_DIR_NOT_EMPTY:
			return X_ENOTEMPTY;

		case ERROR_NO_UNICODE_TRANSLATION:
			return X_EILSEQ;

		case ERROR_INVALID_FUNCTION:
		case ERROR_INVALID_ACCESS:
		case ERROR_INVALID_DATA:
		case ERROR_INVALID_PARAMETER:
		case ERROR_NEGATIVE_SEEK:
			return X_EINVAL;
		default:
			return -winerror;

	}
}

#else

static int eval_errno(int err)
{
	switch(err) {
		case 0:
			return 0;
		case EDQUOT:
			return X_EDQUOT;
		default:
			return errno_is_common(err) ? err : -err;
	}
}

#endif

int x_eval_errno(void)
{
	int error = 0;
#ifdef X_OS_WIN
	error = GetLastError();
#else
	int tmp = errno;
	if (tmp < 0)
		return tmp < -255 ? tmp : - tmp;
	error = errno;
#endif
	if (error)
		errno = eval_errno(error);
	return error;
}

