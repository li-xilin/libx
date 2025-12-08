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

#ifdef __linux__
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <sys/syscall.h>
#include <unistd.h>
#include <fcntl.h>
#endif

#include "x/stat.h"
#include "x/detect.h"
#include "x/uchar.h"
#include <stdlib.h>
#include <stdbool.h>

#ifdef X_OS_WIN
#  ifndef FileIdInfo
#    define FileIdInfo 0x12
#  endif
#  ifndef FSCTL_GET_REPARSE_POINT
#    define FSCTL_GET_REPARSE_POINT 0x900a8
#  endif

typedef struct {
	ULONGLONG   VolumeSerialNumber;
	struct __FILE_ID_128 {
		BYTE Identifier[16];
	} FileId;
} __FILE_ID_INFO;

BOOL (__stdcall* _pfnGetFileInformationByHandleEx)(HANDLE hFile, int FileInformationClass,
		LPVOID lpFileInformation, DWORD dwBufferSize);

static time_t FileTimeToTimet(LPFILETIME pft)
{

	ULARGE_INTEGER time_value;
	time_value.LowPart = pft->dwLowDateTime;
	time_value.HighPart = pft->dwHighDateTime;
	return (time_value.QuadPart - 116444736000000000LL) / 10000000LL ;
}

static BOOL PathIsLink(LPCWSTR pPath)
{
	BOOL retval = FALSE;
	LPVOID pBuffer = NULL;
	HANDLE hFile = INVALID_HANDLE_VALUE;
	DWORD dwSize = 0;

	hFile = CreateFileW(pPath, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_FLAG_OPEN_REPARSE_POINT | FILE_FLAG_BACKUP_SEMANTICS, 0);
	if (hFile == INVALID_HANDLE_VALUE)
		goto out;
	pBuffer = malloc(MAXIMUM_REPARSE_DATA_BUFFER_SIZE);
	if (!pBuffer)
		goto out;

	if (!DeviceIoControl(hFile, FSCTL_GET_REPARSE_POINT, NULL, 0,
				pBuffer, MAXIMUM_REPARSE_DATA_BUFFER_SIZE, &dwSize, NULL))
		goto out;

	if (((REPARSE_GUID_DATA_BUFFER*)pBuffer)->ReparseTag != IO_REPARSE_TAG_SYMLINK)
		goto out;

	retval = FALSE;
out:
	if (hFile != INVALID_HANDLE_VALUE)
		CloseHandle(hFile);
	if (pBuffer)
		free(pBuffer);
	return retval;
}

int x_lstat(const wchar_t* path, x_stat *stat)
{
	int retval = -1;

	__FILE_ID_INFO fileid;
	BY_HANDLE_FILE_INFORMATION info;
	HANDLE hFile = INVALID_HANDLE_VALUE;
	if (!_pfnGetFileInformationByHandleEx) {
		HANDLE hKernel32 = GetModuleHandleW(L"kernel32.dll");
		if (!hKernel32)
			return -1;
		*(FARPROC*)(intptr_t)&_pfnGetFileInformationByHandleEx = GetProcAddress(hKernel32, "GetFileInformationByHandleEx");
		if (!_pfnGetFileInformationByHandleEx)
			return -1;
	}
	hFile = CreateFileW(path, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, 0);
	if (hFile == INVALID_HANDLE_VALUE) {
		DWORD err = GetLastError();
		if (err != ERROR_PATH_NOT_FOUND && err != ERROR_ACCESS_DENIED)
			goto out;
		hFile = CreateFileW(path, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, 0);
		if (hFile == INVALID_HANDLE_VALUE)
			goto out;
	}
	if (!GetFileInformationByHandle(hFile, &info))
		goto out;

	memset(stat, 0, sizeof(x_stat));
	if (_pfnGetFileInformationByHandleEx(hFile, FileIdInfo, &fileid, sizeof(fileid))) {
		stat->st_dev = fileid.VolumeSerialNumber;
		memcpy(stat->st_ino.fileid, fileid.FileId.Identifier, 16);
	}
	else {
		stat->st_dev = info.dwVolumeSerialNumber;
		memset(stat->st_ino.fileid + 0, 0, 8);
		memcpy(stat->st_ino.fileid + 8, &info.nFileIndexHigh, 4);
		memcpy(stat->st_ino.fileid + 12, &info.nFileIndexLow, 4);
	}
	stat->st_atim = FileTimeToTimet(&info.ftLastAccessTime);
	stat->st_ctim = FileTimeToTimet(&info.ftLastAccessTime);
	stat->st_btim = FileTimeToTimet(&info.ftCreationTime);
	stat->st_mtim = FileTimeToTimet(&info.ftLastWriteTime);
	stat->st_uid = stat->st_gid = 0;
	stat->st_nlink = info.nNumberOfLinks;
	ULARGE_INTEGER ui = {
		.HighPart = info.nFileSizeHigh,
		.LowPart = info.nFileSizeLow,
	};
	stat->st_size = ui.QuadPart;
	if (info.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT && PathIsLink(path))
		stat->st_mode |= X_S_IFLNK;
	else if (info.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
		stat->st_mode |= X_S_IFDIR;
	else if (info.dwFileAttributes & FILE_ATTRIBUTE_DEVICE)
		stat->st_mode |= X_S_IFCHR;
	else
		stat->st_mode |= X_S_IFREG;
	retval = 0;
out:
	if (hFile != INVALID_HANDLE_VALUE)
		CloseHandle(hFile);
	return retval;
}

#else
#  include <sys/stat.h>

#if defined(__linux__) && defined(SYS_statx)
#include <linux/stat.h>
#include <fcntl.h>
#endif

static int get_file_creation_time(const char *filename, const struct stat *st, time_t *creat_time)
{

#if defined(__APPLE__) || defined(__FreeBSD__) || defined(__OpenBSD__) || defined(__NetBSD__)
    *creat_time = st.st_birthtime;
	return 0;
#elif defined(st_birthtime)
    *creat_time = st.st_birthtime;
	return 0;
#elif defined(__linux__) && defined(SYS_statx)
	struct statx stx;
	if (syscall(SYS_statx, AT_FDCWD, filename, 0,
				STATX_BTIME, &stx) == 0) {
		if (stx.stx_btime.tv_sec != 0) {
			*creat_time = stx.stx_btime.tv_sec;
			return 0;
		}
	}
	return -1;
#else
   return -1;
#endif
}

int x_lstat(const x_uchar *path, x_stat *stat_buf)
{
	struct stat st;
	if (lstat(path, &st))
		return -1;
	stat_buf->st_uid = st.st_uid;
	stat_buf->st_gid = st.st_gid;
	stat_buf->st_ino = st.st_ino;
	stat_buf->st_dev = st.st_dev;
	stat_buf->st_nlink = st.st_nlink;
	stat_buf->st_size = st.st_size;
	stat_buf->st_mode = st.st_mode;
	stat_buf->st_atim = st.st_atime;
	stat_buf->st_mtim = st.st_mtime;
	stat_buf->st_ctim = st.st_ctime;
	if (get_file_creation_time(path, &st, &stat_buf->st_btim))
		stat_buf->st_btim = st.st_mtime;
	return 0;
}
#endif

