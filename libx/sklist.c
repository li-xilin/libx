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

#include "x/sklist.h"
#include "x/twister.h"
#include "x/dump.h"
#include <stdlib.h>

#define X_SKLIST_PROMOTE(ran) ((ran & 0xffff) <= 0x7fff)

void x_sklist_init(x_sklist *sklist, x_sklist_compare_fn *compare)
{
	sklist->level = 0;
	x_mset_init(&sklist->mset);
	for (int i = 0; i < X_SKLIST_LEVEL_MAX; i++)
		x_list_init(&sklist->list[i]);
	sklist->compare = compare;
	sklist->seed = 1;
}

int x_sklist_random_level(x_sklist *sklist)
{
	int level = 1;
	x_mt19937 mt;
	x_mt19937_init(&mt, sklist->seed);
	while (level <= X_SKLIST_LEVEL_MAX && X_SKLIST_PROMOTE(x_mt19937_next(&mt)))
		level++;
	sklist->seed = x_mt19937_next(&mt); 
	return level;
}

void x_sklist_path_find(x_sklist *sklist, const void *data, x_skpath *path)
{
	size_t identical_level = 0;
	if (!sklist->level) {
		path->identical_level = 0;
		return;
	}
	x_link *end = &sklist->list[sklist->level - 1].head;
	x_link *pos = end;
	for (int i = sklist->level - 1; i >= 0; i--) {
		pos = pos->next;
		for (;pos != end; pos = pos->next) {
			x_sklink *sklink = x_container_of(pos, x_sklink, link[i]);
			int cmp = sklist->compare(sklink->data, data);
			if (cmp > 0) {
				end = pos;
				break;
			}
			else if (cmp == 0) {
				identical_level = i + 1;
				for (int j = 0; j < identical_level; j++)
					path->link[j] = sklink->link[j].next;
				goto out;
			}
		}
		pos = end->prev;
		path->link[i] = end;
		pos--;
		end--;
	}
out:
	path->identical_level = identical_level;
}

void x_sklist_path_insert(x_sklist *sklist, x_skpath *path, void *data)
{
	int link_level = x_sklist_random_level(sklist);
	if (link_level > sklist->level) {
		for (int i = sklist->level; i < link_level; i++) {
			path->link[i] = &sklist->list[i].head;
		}
		sklist->level = link_level;
	}
	x_sklink *sklink = x_malloc(&sklist->mset, sizeof *sklink + link_level * sizeof(x_link));
	sklink->data = data;
	for (int i = 0; i < link_level; i++)
		x_list_insert(path->link[i], &sklink->link[i]);
}

void x_sklist_path_remove(x_sklist *sklist, x_skpath *path)
{
	x_sklink *sklink = x_skpath_matched(path);
	for (int i = 0; i < path->identical_level; i++) {
		x_list_del(&sklink->link[i]);
		if (x_list_is_empty(&sklist->list[i]))
			sklist->level--;
	}
	x_free(sklink);
}

void *x_sklist_find(x_sklist *sklist, const void *data)
{
	x_skpath path;
	x_sklist_path_find(sklist, data, &path);
	x_sklink *sklink = x_skpath_matched(&path);
	if (sklink)
		return sklink->data;
	return NULL;
}

void *x_sklist_find_or_insert(x_sklist *sklist, void *data)
{
	x_skpath path;
	x_sklist_path_find(sklist, data, &path);
	x_sklink *sklink = x_skpath_matched(&path);
	if (sklink)
		return sklink->data;
	x_sklist_path_insert(sklist, &path, data);
	return NULL;
}

void *x_sklist_replace_or_insert(x_sklist *sklist, void *data)
{
	x_skpath path;
	x_sklist_path_find(sklist, data, &path);
	x_sklink *sklink = x_skpath_matched(&path);
	if (sklink) {
		void *orig_data = sklink->data;
		sklink->data = data;
		return orig_data;
	}
	x_sklist_path_insert(sklist, &path, data);
	return NULL;
}

void *x_sklist_remove(x_sklist *sklist, const void *data)
{
	x_skpath path;
	x_sklist_path_find(sklist, data, &path);
	x_sklink *sklink = x_skpath_matched(&path);
	if (!sklink)
		return NULL;
	void *node_data = sklink->data;
	x_sklist_path_remove(sklist, &path);
	return node_data;
}

x_dump *x_sklist_dump(x_sklist *sklist, x_dump *(*print_val)(const void *data))
{
	int cnt = 0;
	if (!sklist->level)
		return x_dump_empty_block("sklist", 0);
	x_list_foreach(cur, &sklist->list[0])
		cnt++;
	x_dump *blk = x_dump_empty_block("sklist", cnt);
	int i = 0;
	x_list_foreach(cur, &sklist->list[0]) {
		x_sklink *sklink = x_container_of(cur, x_sklink, link[0]);
		x_dump_bind(blk, i++, print_val(sklink->data));
	}
	return blk;
}

bool x_sklist_sanitize(x_sklist *sklist)
{
	if (sklist->level > X_SKLIST_LEVEL_MAX)
		return false;
	for (int i = sklist->level + 1; i < X_SKLIST_LEVEL_MAX; i++)
		if (!x_list_is_empty(&sklist->list[i]))
			return false;
	for (int i = 0; i < sklist->level; i++) {
		x_sklink *prev = NULL;
		if (x_list_is_empty(&sklist->list[i]))
			return false;
		x_list_foreach(pos, &sklist->list[i]) {
			x_sklink *sklink = x_container_of(pos, x_sklink, link[i]);
			if (prev && sklist->compare(sklink->data, prev->data) <= 0)
				return false;
			prev = sklink;
		}
	}
	return true;
}
