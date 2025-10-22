/*
 * Copyright (c) 2025 Li Xilin <lixilin@gmx.com>
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
#include "x/fwalker.h"
#include "x/stat.h"
#include "x/dir.h"
#include "x/printf.h"

struct fwalker_dir
{
	x_link link;
	x_dir *dir;
	uint16_t path_len;
};

int x_fwalker_open(x_fwalker *fwalker, const x_uchar *root)
{
	x_mset_init(&fwalker->mset);
	x_list_init(&fwalker->dir_list);
	x_ustrcpy(fwalker->path, root);
	x_path_normalize(fwalker->path);
#ifndef X_OS_WIN
	if (fwalker->path[0] == '/' && fwalker->path[1] == '\0')
		fwalker->path[0] = '\0';
#endif
	return x_fwalker_rewind(fwalker);
}

void x_fwalker_close(x_fwalker *fwalker)
{
	x_list_foreach(cur, &fwalker->dir_list) {
		struct fwalker_dir *f = x_container_of(cur, struct fwalker_dir, link);
		x_dir_close(f->dir);
	}
	x_mset_free(&fwalker->mset);
}

x_uchar *x_fwalker_read(x_fwalker *fwalker, x_stat *statbuf)
{
	struct fwalker_dir *d = NULL;
	x_uchar *ret_path = NULL;
	if (x_list_is_empty(&fwalker->dir_list))
		goto out;
	x_link *last_link = x_list_last(&fwalker->dir_list);
	while (last_link) {
		d = x_container_of(last_link, struct fwalker_dir, link);
		x_dirent *dirent = x_dir_read(d->dir);
		if (dirent) {
			if (dirent->d_name[0] == '.') {
				if (dirent->d_name[1] == '\0')
					continue;
				if (dirent->d_name[1] == '.' && dirent->d_name[2] == '\0')
					continue;
			}
			x_sprintf(fwalker->path + d->path_len, x_u("/%S"), dirent->d_name);
			if (x_lstat(fwalker->path, statbuf))
				continue;
			if ((statbuf->st_mode & X_S_IFMT) == X_S_IFDIR) {
				x_dir *dir = x_dir_open(fwalker->path);
				if (!dir)
					continue;
				struct fwalker_dir *new_dir = x_malloc(&fwalker->mset,  sizeof *new_dir);
				new_dir->dir = dir;
				new_dir->path_len = d->path_len + x_ustrlen(dirent->d_name) + 1;
				x_list_add_back(&fwalker->dir_list, &new_dir->link);
			}
			ret_path = fwalker->path;
			goto out;
		}
		x_dir_close(d->dir);
		x_list_del(&d->link);
		fwalker->path[d->path_len - !x_list_is_empty(&fwalker->dir_list)] = '\0';
		x_free(d);
		last_link = x_list_last(&fwalker->dir_list);
	}
out:
	return ret_path;
}

int x_fwalker_rewind(x_fwalker *fwalker)
{
	const x_uchar *root_path = fwalker->path;
#ifndef X_OS_WIN
	if (fwalker->path[0] == '\0')
		root_path = "/";
#endif
	x_dir *dir = x_dir_open(root_path);
	if (!dir)
		return -1;
	if (!x_list_is_empty(&fwalker->dir_list)) {
		x_link *first_link = x_list_first(&fwalker->dir_list);
		struct fwalker_dir *d  = x_container_of(first_link, struct fwalker_dir, link);
		fwalker->path[d->path_len] = '\0';
	}
	x_list_foreach(cur, &fwalker->dir_list) {
		struct fwalker_dir *f = x_container_of(cur, struct fwalker_dir, link);
		x_dir_close(f->dir);
	}
	x_mset_clear(&fwalker->mset);
	x_list_init(&fwalker->dir_list);

	struct fwalker_dir *d = x_malloc(&fwalker->mset,  sizeof *d);
	d->dir = dir;
	d->path_len = x_ustrlen(fwalker->path);
	x_list_add_back(&fwalker->dir_list, &d->link);
	return 0;
}

void x_fwalker_leave(x_fwalker *fwalker)
{
	if (x_list_is_empty(&fwalker->dir_list))
		return;
	x_link *last_link = x_list_last(&fwalker->dir_list);
	struct fwalker_dir *d = x_container_of(last_link, struct fwalker_dir, link);
	x_dir_close(d->dir);
	x_list_del(&d->link);
	fwalker->path[d->path_len - !x_list_is_empty(&fwalker->dir_list)] = '\0';
	x_free(d);
}

