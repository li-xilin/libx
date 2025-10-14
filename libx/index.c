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
#include "x/index.h"

static int cmp_index_list(const x_btnode *x, const x_btnode *y);

void x_index_init(x_index *idx)
{
	assert(idx);
	idx->ilist = NULL;
}

void x_index_insert(x_index *idx, x_indexer *indexer)
{
	assert(idx);
	assert(indexer);
	x_indexset *ilist = idx->ilist;
	if (ilist) {
		if (!x_list_has_multiple(&ilist->entries))
			x_splay_remove(&ilist->indexer->indexes, &ilist->node);
		else
			ilist = NULL;
		x_list_del(&idx->link);
		idx->ilist = NULL;
	}
	x_indexset tmp;
	x_list_init(&tmp.entries);
	x_list_add_front(&tmp.entries, &idx->link);
	x_btnode *result = x_splay_find(&indexer->indexes, &tmp.node);
	if (result) {
		x_indexset *existed_ilist = x_container_of(result, x_indexset, node);
		x_list_add_back(&existed_ilist->entries, &idx->link);
		idx->ilist = existed_ilist;
		x_free(ilist);
	}
	else {
		if (!ilist) {
			ilist = x_malloc(&indexer->mset, sizeof *ilist);
			x_list_init(&ilist->entries);
			ilist->indexer = indexer;
		}
		x_list_add_front(&ilist->entries, &idx->link);
		x_splay_find_or_insert(&indexer->indexes, &ilist->node);
		idx->ilist = ilist;
	}
}

void x_index_remove(x_index *idx)
{
	assert(idx);
	x_indexset *ilist = idx->ilist;
	if (!ilist)
		return;
	x_list_del(&idx->link);
	if (x_list_is_empty(&ilist->entries)) {
		x_splay_remove(&ilist->indexer->indexes, &ilist->node);
		x_free(ilist);
	}
	idx->ilist = NULL;
}

static int cmp_index_list(const x_btnode *x, const x_btnode *y)
{
	x_indexset *l1 = x_container_of(x, x_indexset, node);
	x_indexset *l2 = x_container_of(y, x_indexset, node);
	x_index *idx1 = x_container_of(x_list_first(&l1->entries), x_index, link);
	x_index *idx2 = x_container_of(x_list_first(&l2->entries), x_index, link);
	return l2->indexer->cmp(idx1, idx2);
}

void x_indexer_init(x_indexer *indexer, x_index_cmp_f *cmp)
{
	assert(indexer);
	assert(cmp);
	x_splay_init(&indexer->indexes, cmp_index_list);
	indexer->cmp = cmp;
	x_mset_init(&indexer->mset);
}

const x_indexset *x_indexer_find(x_indexer *indexer, const x_index *pattern)
{
	assert(indexer);
	assert(pattern);
	x_indexset tmp;
	x_list_init(&tmp.entries);
	tmp.entries.head.prev = (x_link *)&pattern->link;
	tmp.entries.head.next = (x_link *)&pattern->link;
	x_btnode *result = x_splay_find(&indexer->indexes, &tmp.node);
	if (!result)
		return NULL;
	return x_container_of(result, x_indexset, node);
}

x_index *x_indexset_first(const x_indexset *iset)
{
	assert(iset);
	x_link *first = x_list_first(&iset->entries);
	if (!first)
		return NULL;
	return x_container_of(first, x_index, link);
}

x_index *x_indexset_next(const x_index *idx)
{
	assert(idx);
	x_link *next = idx->link.next;
	if (next == &idx->ilist->entries.head)
		return NULL;
	return x_container_of(next, x_index, link);
}

