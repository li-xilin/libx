/*
 * Copyright (c) 2022-2024 Li Xilin <lixilin@gmx.com>
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

#include "def.h"
#include <assert.h>

#ifndef X_LINK_DEFINED
#define X_LINK_DEFINED
typedef struct x_link_st x_link;
#endif

#ifndef X_LIST_DEFINED
#define X_LIST_DEFINED
typedef struct x_list_st x_list;
#endif

struct x_link_st
{
	x_link *prev, *next;
};

struct x_list_st
{
	x_link head;
};

#define X_LIST_INIT(name) { &(name), &(name) }

inline static void x_list_init(x_list *list)
{
        list->head.next = list->head.prev = &list->head;
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

inline static void x_list_add_front(x_link *head, x_link *new_link)
{
        __x_list_add(new_link, head, head->next);
}

inline static void x_list_add_back(x_list *list, x_link *new_link)
{
        __x_list_add(new_link, list->head.prev, &list->head);
}

inline static void __x_list_del(x_link * prev, x_link * next)
{
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

inline static x_link *x_list_first(x_list *list)
{
	return x_list_is_empty(list) ? NULL : list->head.next;
}

inline static x_link *x_link_last(x_list *list)
{
	return x_list_is_empty(list) ? NULL : list->head.prev;
}

#define x_list_foreach(pos, list) \
	for ((pos) = (list)->head.next; (pos) != &(list)->head; (pos) = (pos)->next)

#endif
