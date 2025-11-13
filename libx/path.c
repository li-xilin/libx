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

#include "x/path.h"
#include "x/sys.h"
#include "x/detect.h"
#include "x/string.h"

#include <errno.h>
#include <stdarg.h>
#include <limits.h>
#include <stdlib.h>
#include <stdbool.h>
#include <ctype.h>

#ifdef X_OS_WIN
#include <windows.h>
#include <wchar.h>
#else
#include <unistd.h>
#endif

static int path_rtrim(x_uchar *path);

static int path_rtrim(x_uchar *path)
{
	int len = x_ustrlen(path);
	for (int i = len - 1; i != -1; i--) {
		if (path[i] != X_PATH_SEP_CHAR)
			break;
		len--;
	}
	path[len] = x_u('\0');
	return len;
}

void x_path_empty(x_uchar *path)
{
	x_ustrcpy(path, x_u(""));
}

x_uchar *x_path_push(x_uchar *path, size_t size, const x_uchar *name)
{
	int path_len = x_ustrlen(path);
	int name_len = x_ustrlen(name);

	bool need_sep = true;
	if (path_len == 0)
		need_sep = false;
	if (path[path_len - 1] == X_PATH_SEP_CHAR)
		need_sep = false;

	if (path_len + name_len + !!need_sep + 1 > size) {
		errno = ENOBUFS;
		return NULL;
	}
	if (need_sep)
		x_ustrcpy(path + path_len, X_PATH_SEP);
	x_ustrcpy(path + path_len + !!need_sep, name);
	return path;
}

x_uchar *x_path_trim(x_uchar *path)
{
	if (!path)
		return NULL;

	bool pre_is_sep = false;
	x_uchar *front = path, *rear = path;

	while (*front) {
		if (*front == X_PATH_SEP_CHAR) {
			if (pre_is_sep) {
				front++;
				continue;
			}
			pre_is_sep = true;
		}
		else
			pre_is_sep = false;
		*rear++ = *front++;
	}
	if (rear != path && rear != path + 1 && rear[-1] == X_PATH_SEP_CHAR) {
		rear[-1] = x_u('\0');
	}
	else
		*rear = x_u('\0');
	return path;
}

bool x_path_is_absolute(const x_uchar *path)
{
	return !!x_path_root_len(path);
}

const x_uchar *x_path_pop(x_uchar *path)
{
	if (!path)
		return NULL;

	int len = path_rtrim(path);

	for (int i = len - 1; i != -1; i--) {
		if (path[i] == X_PATH_SEP_CHAR) {
			path[i] = x_u('\0');
			return path + i + 1;
		}
	}

	if (!path[0]) {
		return NULL;
	}

	memmove(path + 1, path, (len + 1) * sizeof(x_uchar));
	path[0] = x_u('\0');
	return path + 1;
}

x_uchar *x_path_join(x_uchar *path, size_t size, ...)
{
	va_list ap;
	va_start(ap, size);
	x_uchar *name = va_arg(ap, x_uchar *);
	while (name) {
		if (!x_path_push(path, size, name))
			return NULL;
		name = va_arg(ap, x_uchar *);
	}
	return path;
}

x_uchar *x_path_fixsep(x_uchar *path)
{
	if (!path)
		return NULL;
#ifdef X_OS_WIN32
	for (int i = 0; path[i]; i++)
		if (path[i] == L'/')
			path[i] = L'\\';
#endif
	return path;
}

int x_path_root_len(const x_uchar *path)
{
	int len = 0;
#ifdef X_OS_WIN
	if (isalpha(path[0]) && path[1] == L':' ) {
		if (path[2] == x_u('\\'))
			len = 3;
		else if (path[2] == x_u('\0'))
			len = 2;
	}
#endif
	int sep_cnt;
	for (sep_cnt = 0; path[len + sep_cnt] && path[len + sep_cnt] == X_PATH_SEP_CHAR; sep_cnt++);
	return len + sep_cnt;
}

x_uchar *x_path_normalize(x_uchar *path)
{
	if (!x_path_trim(x_path_fixsep(path)))
		return NULL;

	int root_len = x_path_root_len(path);
	x_uchar *p = path + root_len;
	
	x_uchar *nam_tab[X_PATH_MAX / 2];
	int name_cnt = 0;

	/* Split the path sequence by / or \ */
	x_uchar *name;
	while ((name = x_ustrsplit(&p, X_PATH_SEP_CHAR))) {
		nam_tab[name_cnt] = name;
		name_cnt++;
	}
	nam_tab[name_cnt] = NULL;

	/* Remove duplicate hard link . and .. , but which begin with relative path are essential */
	int front = 0, rear = 0;
	for ( ;nam_tab[front]; front++) {
		if (x_ustrcmp(nam_tab[front], x_u(".")) == 0)
			continue;

		else if (x_ustrcmp(nam_tab[front], x_u("..")) == 0) {
			if (root_len) {
				if (rear)
					rear--;
				continue;
			}
			else {
				if (rear && x_ustrcmp(nam_tab[rear - 1], x_u("..")) != 0) {
					rear--;
					continue;
				}
			}
		}
		nam_tab[rear++] = nam_tab[front];
	}
	nam_tab[rear] = NULL;

	/* Construct new path sequence by name segments */
	if (nam_tab[0]) {
		memmove(path + root_len, nam_tab[0], x_ustrlen(nam_tab[0]) + 1);
		for (int i = 1; nam_tab[i]; i++) {
			int len = x_ustrlen(path + root_len);
			path[root_len + len] = X_PATH_SEP_CHAR;
			memmove(path + root_len + len + 1, nam_tab[i], (x_ustrlen(nam_tab[i]) + 1) * sizeof(x_uchar));
		}
	}
	return path;
}

x_uchar *x_path_realize(const x_uchar *path, x_uchar *resolved_path, size_t size)
{
#ifdef X_OS_WIN
	DWORD dwLen = GetFullPathNameW(path, size, resolved_path, NULL);
	if (dwLen == 0) {
		return NULL;
	}
	if (resolved_path[dwLen - 1] == L'\\')
		resolved_path[dwLen - 1] = L'\0';

	return resolved_path;
#else
	char resolved_buf[PATH_MAX];
	if (!realpath(path, resolved_buf))
		return NULL;
	int len = strlen(resolved_buf);
	if (len + 1 > size) {
		errno = ENOBUFS;
		return NULL;
	}
	memcpy(resolved_path, resolved_buf, len + 1);
	return resolved_path;
#endif
}

const x_uchar *x_path_basename(const x_uchar *path)
{
	int i;
	int len = x_ustrlen(path);
	for (i = len - 1; i != -1; i--) {
		if (path[i] == X_PATH_SEP_CHAR)
			return path + i + 1;
	}
	return path;
}

const x_uchar *x_path_extname(const x_uchar *path)
{
	int i;
	int len = x_ustrlen(path);
	for (i = len - 1; i != -1; i--) {
		if (path[i] == X_PATH_SEP_CHAR)
			return path + len;

		if (path[i] == x_u('.'))
			break;
	}

	if (i > 0 && path[i - 1] != X_PATH_SEP_CHAR)
		return path + i + 1;
	else
		return path + len;
}

x_uchar *x_path_getcwd(x_uchar *path, size_t size)
{
#ifdef X_OS_WIN
	return _wgetcwd(path, size);
#else
	return getcwd(path, size);
#endif
}

int x_path_setcwd(const x_uchar *path)
{
#ifdef X_OS_WIN
	return _wchdir(path);
#else
	return chdir(path);
#endif
}

x_uchar *x_path_homedir(x_uchar *path, size_t size)
{
	const x_uchar *home_dir = x_sys_getenv(
#ifdef X_OS_WIN
	L"USERPROFILE"
#else
	"HOME"
#endif
	);
	if (!home_dir) {
		errno = ENOENT;
		return NULL;
	}
	int len = x_ustrlen(home_dir);

	if (len + 1 > size) {
		errno = ENOBUFS;
		return NULL;
	}
	memcpy(path, home_dir, (len + 1) * sizeof(x_uchar));
	return path;
}

x_uchar *x_path_tmpdir(x_uchar *path, size_t size)
{
#ifdef X_OS_WIN
	DWORD dwLen = GetTempPathW(size, path);
	if (dwLen == 0)
		return NULL;
#else
#define UNIX_TEMP_PATH "/tmp"
	if (size < sizeof UNIX_TEMP_PATH) {
		errno = ENOBUFS;
		return NULL;
	}
	
	strcpy(path, UNIX_TEMP_PATH);
#endif
	return path;
}

