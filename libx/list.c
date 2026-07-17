#include "x/list.h"

void x_list_merge_front(x_list *dest, x_list *src)
{
	assert(dest);
	assert(src);
	if (!x_list_is_empty(src)) {
		src->head.prev->next = dest->head.next;
		src->head.next->prev = &dest->head;
		dest->head.next = src->head.next;
		dest->head.next->prev = src->head.prev;
	}
}

void x_list_merge_back(x_list *dest, x_list *src)
{
	assert(dest);
	assert(src);
	if (!x_list_is_empty(src)) {
		src->head.prev->next = &dest->head;
		src->head.next->prev = dest->head.prev;
		dest->head.prev->next = src->head.next;
		dest->head.prev = src->head.prev;
	}
}
