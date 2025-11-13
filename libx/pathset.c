/*
 * Copyright (c) 2025 Li Xilin <lixilin@gmx.com>
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
#include "x/pathset.h"
#include "x/string.h"
#include "x/path.h"
#include "x/errno.h"
#include "x/strbuf.h"
#include "x/printf.h"
#include "x/detect.h"

#define PATH_DEPTH_MAX 256

typedef struct path_mark_st path_mark;
typedef struct pattern_st pattern;

struct pattern_st {
	uint32_t hash_cnt;
	uint32_t hashs[PATH_DEPTH_MAX];
	uint16_t path_len;
	x_uchar path[X_PATH_MAX];
};

struct path_mark_st
{
	x_link link;
	uint32_t mask;
	uint32_t hash;
	uint16_t path_len;
	uint8_t path_deepth;
	bool is_leaf;
	x_uchar path[];
};

static bool path_mark_match(path_mark *mark, const pattern *pat);
static bool path_mark_contain(path_mark *mark, const pattern *pat, bool with_equal);
static uint32_t x_pathset_transmit(x_pathset *pset, const path_mark *mark, uint32_t mask, bool is_leaf, bool add);

static bool path_mark_set_mask(path_mark *mark, uint32_t mask, bool is_leaf, bool add_mark) {
	uint32_t origin_mask = mark->mask;
	if (add_mark) 
		mark->mask |= mask;
	else
		mark->mask &= ~mask;
	return mark->mask != origin_mask;
}

static path_mark *path_mark_alloc(x_pathset *pset, uint32_t mask, const pattern *pat, bool is_leaf)
{
	size_t path_size = (pat->path_len + 1 ) * sizeof(x_uchar);
	path_mark *mark = x_malloc(&pset->mset, sizeof *mark + path_size);
	mark->hash = pat->hashs[pat->hash_cnt - 1];
	mark->mask = mask;
	mark->path_len = pat->path_len;
	memcpy(mark->path, pat->path, path_size);
	mark->is_leaf = is_leaf;
	mark->path_deepth = pat->hash_cnt;
	return mark;
}

static int pattern_init(pattern *pat, const x_uchar *path)
{
	pat->path_len = x_ustrlen(path);
	if (pat->path_len > X_PATH_MAX - 1) {
		errno = X_ENAMETOOLONG;
		return -1;
	}
	x_ustrcpy(pat->path, path);
	x_path_normalize(pat->path);
	if (pat->path[0] != X_PATH_SEP_CHAR) {
		errno = X_EINVAL;
		return -1;
	}
#ifndef X_OS_WIN
	bool restore_root = false;
	if (pat->path[1] == '\0' && pat->path[0] == X_PATH_SEP_CHAR) {
		pat->path[0] = '\0';
		restore_root = true;
	}
#endif
	x_uchar *state = pat->path, *name;
	uint32_t hash = 0;
	pat->hash_cnt = 0;
	while ((name = x_ustrsplit(&state, X_PATH_SEP_CHAR))) {
		if (pat->hash_cnt == PATH_DEPTH_MAX) {
			errno = X_ENAMETOOLONG;
			return -1;
		}
		hash ^= x_ustrhash(name);
		pat->hashs[pat->hash_cnt++] = hash;
		if (name != pat->path)
			name[-1] = X_PATH_SEP_CHAR;
	}
#ifndef X_OS_WIN
	if (restore_root) {
		pat->path[0] = X_PATH_SEP_CHAR;
	}
#endif
	return 0;
}

void x_pathset_init(x_pathset *pset)
{
	pset->unitary_mask = 0;
	x_list_init(&pset->dir_list);
	x_rwlock_init(&pset->lock);
	x_mset_init(&pset->mset);
}

void x_pathset_free(x_pathset *pset)
{
	x_pathset_clear(pset);
	x_rwlock_destroy(&pset->lock);
	x_mset_free(&pset->mset);
}

static path_mark *find_parent(x_pathset *pset, const pattern  *pat)
{
	path_mark *mark = NULL;
	x_list_foreach(pos, &pset->dir_list) {
		path_mark *cur = x_container_of(pos, path_mark, link);
		if (cur->is_leaf) {
			if (path_mark_match(cur, pat))
				return cur;
		}
		else if (path_mark_contain(cur, pat, false)) {
			if (!mark) {
				mark = cur;
				continue;
			}
			if (mark->path_len < cur->path_len)
				mark = cur;
		}
	}
	return mark;
}

static bool path_mark_contain(path_mark *mark, const pattern *pat, bool with_equal)
{
	if (pat->hash_cnt< mark->path_deepth + (unsigned)!!with_equal )
		return false;
	if (mark->hash != pat->hashs[mark->path_deepth - 1])
		return false;
	if (x_ustrncmp(pat->path, mark->path, mark->path_len) != 0)
		return false;
#ifndef X_OS_WIN
	if (mark->path[0] && mark->path[0] == X_PATH_SEP_CHAR && mark->path[1] == '\0')
		return true;
#endif
	int last_char = pat->path[mark->path_len];
	if (last_char != '\0' && last_char != '/')
		return false;
	return true;
}

static bool path_mark_match(path_mark *mark, const pattern *pat)
{
	if (pat->hash_cnt != mark->path_deepth)
		return false;
	if (mark->hash != pat->hashs[mark->path_deepth - 1])
		return false;
	if (x_ustrcmp(pat->path, pat->path) != 0)
		return false;
	return true;
}

static bool path_mark_above(const path_mark *mark, const path_mark *mark1)
{
	if (mark1->path_deepth >= mark->path_deepth)
		return false;
	if (x_ustrncmp(mark1->path, mark->path, mark1->path_len) != 0)
		return false;
	if (mark->path[mark1->path_len] != '\0' && mark->path[mark1->path_len] != '/')
		return false;
	return true;
}

static uint32_t x_pathset_update(x_pathset *pset, const pattern *pat, uint32_t mask, bool is_leaf, bool add)
{
	uint32_t origin_mask = 0;
	path_mark *mark = NULL;
	x_list_foreach(pos, &pset->dir_list) {
		path_mark *cur = x_container_of(pos, path_mark, link);
		if (cur->is_leaf != is_leaf)
			continue;
		if (cur->is_leaf) {
			if (path_mark_match(cur, pat)) {
				mark = cur;
				break;
			}
		}
		else if (path_mark_contain(cur, pat, false)) {
			if (!mark) {
				mark = cur;
				continue;
			}
			if (mark->path_len < cur->path_len)
				mark = cur;
		}
	}
	if (mark) {
		if (pat->path_len == mark->path_len) {
			path_mark_set_mask(mark, mask, is_leaf, add);
			return x_pathset_transmit(pset, mark, mask, is_leaf, add);
		}
		origin_mask = mark->mask;
	}
	path_mark *new_mark = path_mark_alloc(pset, origin_mask, pat, is_leaf);
	if (path_mark_set_mask(new_mark, mask, is_leaf, add))
		x_list_add_back(&pset->dir_list, &new_mark->link);
	else
		x_free(new_mark);
	return x_pathset_transmit(pset, new_mark, mask, is_leaf, add);
}

uint32_t x_pathset_insert(x_pathset *pset, uint32_t mask, const x_uchar *path, bool is_leaf)
{
	pattern pat;
	if (pattern_init(&pat, path))
		return 0;
	x_rwlock_wlock(&pset->lock);
	uint32_t ret = x_pathset_update(pset, &pat, mask, is_leaf, true);
	x_rwlock_unlock(&pset->lock);
	return ret;
}

uint32_t x_pathset_remove(x_pathset *pset, uint32_t mask, const x_uchar *path, bool is_leaf)
{
	pattern pat;
	if (pattern_init(&pat, path))
		return 0;
	x_rwlock_wlock(&pset->lock);
	uint32_t ret = x_pathset_update(pset, &pat, mask, is_leaf, false);
	x_rwlock_unlock(&pset->lock);
	return ret;
}

uint32_t x_pathset_mask(x_pathset *pset, const x_uchar *path)
{
	uint32_t ret = 0;
	pattern pat;
	if (pattern_init(&pat, path))
		return 0;
	x_rwlock_rlock(&pset->lock);
	path_mark *mark = find_parent(pset, &pat);
	if (!mark)
		goto unlock;
	if (pat.path_len == mark->path_len && !mark->is_leaf)
		goto unlock;
	ret = mark->mask;
unlock:
	x_rwlock_unlock(&pset->lock);
	return ret;
}

void x_pathset_clear(x_pathset *pset)
{
	x_rwlock_wlock(&pset->lock);
	while (!x_list_is_empty(&pset->dir_list)) {
		x_link *pos = x_list_first(&pset->dir_list);
		path_mark *mark = x_container_of(pos, path_mark, link);
		x_list_del(pos);
		x_free(mark);
	}
	x_rwlock_unlock(&pset->lock);
}

uint32_t x_pathset_unitary_mask(x_pathset *pset)
{
	return pset->unitary_mask;
}

static uint32_t x_pathset_transmit(x_pathset *pset, const path_mark *mark, uint32_t mask, bool is_leaf, bool add)
{
	uint32_t old_mask = pset->unitary_mask;
	if (is_leaf) {
		if (add) {
			pset->unitary_mask |= mask;
			return (old_mask | mask) ^ old_mask;
		}
		else {
			pset->unitary_mask &= ~mask;
			return (old_mask & ~mask) ^ old_mask;
		}
	}
	pset->unitary_mask = 0;
	for (x_link *pos = x_list_first(&pset->dir_list); pos != &pset->dir_list.head; ) {
		path_mark *cur = x_container_of(pos, path_mark, link);
		pos = pos->next;
		pset->unitary_mask |= cur->mask;
		if (!path_mark_above(cur, mark))
			continue;
		path_mark_set_mask(cur, mask, is_leaf, add);
		if (cur->mask != mark->mask)
			continue;
		x_list_del(&cur->link);
		x_free(cur);
	}
	return pset->unitary_mask ^ old_mask;
}

void x_pathset_dump(x_pathset *pset, x_strbuf *strbuf)
{
	x_rwlock_rlock(&pset->lock);
	x_strbuf_init(strbuf);
	x_uchar buf[X_PATH_MAX + 64];
	x_list_foreach(pos, &pset->dir_list) {
		path_mark *mark = x_container_of(pos, path_mark, link);
		x_sprintf(buf, x_u("0x%02I32d %s%S, "), mark->mask, (mark->path_len == 0 ? "/" : ""), mark->path);
		x_strbuf_append(strbuf, buf);
	}
	x_rwlock_unlock(&pset->lock);
}

void x_pathset_find_top(x_pathset *pset, x_list *list)
{
	x_rwlock_rlock(&pset->lock);
	x_list_init(list);
	x_list_foreach(pos, &pset->dir_list) {
		path_mark *mark = x_container_of(pos, path_mark, link);
		x_list_foreach(pos_res, list) {
			x_pathset_entry *top = x_container_of(pos_res, x_pathset_entry, link);
			if (top->path_len < mark->path_len) {
				if (x_ustrncmp(top->path, mark->path, top->path_len) != 0)
					continue;
				if (mark->path[top->path_len] != '\0' && mark->path[top->path_len] != X_PATH_SEP_CHAR)
					continue;
				goto next_mark;
			}
			else {
				if (x_ustrncmp(top->path, mark->path, mark->path_len) != 0)
					continue;
				if (top->path[mark->path_len] != '\0' && top->path[mark->path_len] != X_PATH_SEP_CHAR)
					continue;
				top->path[mark->path_len] = '\0';
				top->path_len = mark->path_len;
				goto next_mark;
			}
		}
		x_pathset_entry *top = x_malloc(&pset->mset, sizeof *top + (mark->path_len + 1 ) * sizeof(x_uchar));
		top->path_len = mark->path_len;
		x_ustrcpy(top->path, mark->path);
		x_list_add_back(list, &top->link);
next_mark:
		;
	}
	x_rwlock_unlock(&pset->lock);
}

void x_pathset_free_entry_list(x_list *list)
{
	x_list_popeach(pos, list)
		x_free(x_container_of(pos, x_pathset_entry, link));
	x_list_init(list);
}

