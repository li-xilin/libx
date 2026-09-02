/*
 * Copyright (c) 2022-2026 Li Xilin <lixilin@gmx.com>
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

#ifndef X_LIST_H
#define X_LIST_H

#include "types.h"
#include "macros.h"
#include <assert.h>

#define X_LIST_INIT(name) { { &(name.head), &(name.head) } }
#define X_SLIST_INIT { NULL }

#define x_list_foreach(pos, list) \
	for (x_link *pos = (list)->head.next; (pos) != &(list)->head; (pos) = (pos)->next)

#define x_list_popeach(pos, list) \
	for (x_link *pos, __x_cur; (pos) = (list)->head.next, __x_cur = *(pos), !x_list_is_empty(list); x_list_del(&__x_cur))

#define x_slist_foreach(pos, list) \
	for (x_slink *pos = (list)->head.next; pos; (pos) = (pos)->next)

#define x_slist_foreach_prev(prev, list) \
	for (x_slink *prev = &(list)->head, *__x_cur; prev->next && (__x_cur = (prev)->next, 1); (prev) = __x_cur)

#define x_slist_popeach(pos, list) \
	for (x_slink *pos; (pos = (list)->head.next) && ((list)->head.next = pos->next, 1); )

struct x_link_st {
	x_link *prev, *next;
};

struct x_list_st {
	x_link head;
};

struct x_slink_st {
	x_slink *next;
};

struct x_slist_st {
	x_slink head;
};

inline static void x_list_init(x_list *list)
{
	list->head.next = list->head.prev = &list->head;
}

inline static void x_link_init(x_link *link)
{
	link->prev = link->next = NULL;
}

inline static void x_list_swap(x_link *link1, x_link *link2)
{
	x_link *p = NULL, *q;
	if (link1->next == link2)
		p = link1, q = link2;
	else if (link2->next == link1)
		p = link2, q = link1;

	if (p) {
		q->next->prev = p;
		p->prev->next = q;
		p->next = q->next;
		q->prev = p->prev;
		p->prev = p;
		q->next = q;
	}
	else {
		x_link tmp = *link1;
		link1->prev->next = link2;
		link1->next->prev = link2;
		link2->prev->next = link1;
		link1->next->prev = link1;
		*link1 = *link2;
		*link2 = tmp;
	}
}

inline static void x_list_replace(x_link *link, x_link *new_link)
{
	link->prev->next = new_link;
	link->next->prev = new_link;
	*new_link = *link;
}

inline static void x_list_clear(x_list *list)
{
	list->head.next = list->head.prev = NULL;
}

inline static void __x_list_add(x_link *new_link, x_link *prev, x_link *next)
{
	next->prev = new_link;
	new_link->next = next;
	new_link->prev = prev;
	prev->next = new_link;
}

inline static void x_list_add_front(x_list *list, x_link *new_link)
{
	assert(list);
	assert(new_link);
	__x_list_add(new_link, &list->head, list->head.next);
}

inline static void x_list_add_back(x_list *list, x_link *new_link)
{
	assert(list);
	assert(new_link);
	__x_list_add(new_link, list->head.prev, &list->head);
}

inline static void x_list_insert(x_link *link, x_link *new_link)
{
	assert(link);
	assert(new_link);
	__x_list_add(new_link, link->prev, link);
}

inline static void __x_list_del(x_link * prev, x_link * next)
{
	assert(prev);
	assert(next);
	next->prev = prev;
	prev->next = next;
}

inline static void x_list_del(x_link *link)
{
	assert(link);
	assert(link->prev);
	assert(link->next);
	__x_list_del(link->prev, link->next);
	link->prev = link->next = 0;
}


inline static int x_list_is_empty(const x_list *list)
{
	return list->head.next == &list->head;
}


inline static x_link *x_list_first(const x_list *list)
{
	return x_list_is_empty(list) ? NULL : list->head.next;
}

inline static x_link *x_list_last(const x_list *list)
{
	return x_list_is_empty(list) ? NULL : list->head.prev;
}

inline static bool x_list_has_multiple(const x_list *list)
{
	return !x_list_is_empty(list) && x_list_first((x_list *)list) != x_list_last((x_list *)list);
}

inline static x_link *x_list_second(const x_list *list)
{
	return x_list_has_multiple(list) ? list->head.next->next : NULL;
}

void x_list_merge_front(x_list *dest, x_list *src);
void x_list_merge_back(x_list *dest, x_list *src);

inline static void x_slist_init(x_slist *list)
{
	list->head.next = NULL;
}

inline static void x_slist_add_front(x_slist *list, x_slink *link)
{
	link->next = list->head.next;
	list->head.next = link;
}

inline static x_slink *x_slist_del_front(x_slist *list)
{
	if (!list->head.next)
		return NULL;
	x_slink *first = list->head.next;
	list->head.next = list->head.next->next;
	return first;
}

inline static x_slink *x_slist_first(x_slist *list, x_slink *link)
{
	return list->head.next;
}

inline static bool x_slist_is_empty(const x_slist *list)
{
	return !list->head.next;
}

inline static void x_slink_insert_after(x_slink *link, x_slink *new_link)
{
	new_link->next = link->next;
	link->next = new_link;
}

inline static x_slink *x_slink_del_after(x_slink *link)
{
	if (!link->next)
		return NULL;
	x_slink *del = link->next;
	link->next = link->next->next;
	return del;
}

inline static x_slink *x_slink_replace_after(x_slink *link, x_slink *new_link)
{
	if (!link->next)
		return NULL;
	x_slink *del = link->next;
	new_link->next = link->next;
	link->next = new_link;
	return del;
}

#endif

