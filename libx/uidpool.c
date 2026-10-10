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
#include "x/uidpool.h"
#include "x/errno.h"

int x_uidpool_compare_uid(const x_uidlink *a, const x_uidlink *b)
{
	const x_uidlink *u1 = a, *u2 = b;
	if (u1->num < u2->num)
		return -1;
	if (u1->num > u2->num)
		return 1;
	return 0;
}

void x_uidpool_init1(x_uidpool *pool, uint32_t max_uid)
{
	x_sklist_init(&pool->uid_set, (x_sklist_compare_fn *)x_uidpool_compare_uid);
	pool->uid_count = 0;
	pool->next_uid = 1;
	pool->max_uid = max_uid == 0 ? UINT32_MAX : max_uid;
}

void x_uidpool_init(x_uidpool *pool)
{
	x_uidpool_init1(pool, 0);
}

void x_uidpool_free(x_uidpool *pool)
{
	x_sklist_free(&pool->uid_set);
}

static bool increase_uid(x_uidpool *pool)
{
	if (++pool->next_uid == pool->max_uid + 1) {
		pool->next_uid = 1;
		return true;
	}
	return false;
}

int x_uidpool_get(x_uidpool *pool, x_uidlink *link)
{
	x_skpath path;
	if (pool->uid_count == pool->max_uid) {
		errno = X_ENOMEM;
		return -1;
	}
	if (!x_sklist_path_find(&pool->uid_set, x_container_of(&pool->next_uid, x_uidlink, num), &path)) {
		link->num = pool->next_uid;
		x_sklist_path_insert(&pool->uid_set, &path, link);
		goto out;
	}
	increase_uid(pool);
	for (x_slink *pos = path.link[0]->next; pos->next; ) {
		x_uidlink *uid = x_container_of(pos->next, x_sklink, link[0])->data;
		if (pool->next_uid != uid->num)
			break;
		if (increase_uid(pool)) {
			pos = &pool->uid_set.list[0].head;
			continue;
		}
		pos = pos->next;
	}
	link->num = pool->next_uid;
	void *origin = x_sklist_find_or_insert(&pool->uid_set, link);
	assert(!origin);
	x_unused(origin);
out:
	pool->uid_count++;
	if (++pool->next_uid == pool->max_uid + 1)
		pool->next_uid = 1;
	return 0;
}

void x_uidpool_put(x_uidpool *pool, x_uidlink *link)
{
	x_skpath path;
	if (!x_sklist_path_find(&pool->uid_set, link, &path))
		return;
	x_uidlink *res = x_container_of(path.link[0]->next, x_sklink, link[0])->data;
	if (res != link)
		return;
	x_sklist_path_remove(&pool->uid_set, &path);
	pool->uid_count--;
	link->num = X_BADUID;
}

x_uidlink *x_uidpool_find(x_uidpool *pool, uint32_t uid)
{
	return x_sklist_find(&pool->uid_set, x_container_of(&uid, x_uidlink, num));
}

