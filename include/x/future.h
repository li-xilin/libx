/*
  Copyright (c) 2025 Li Xilin <lixilin@gmx.com>

  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in
  all copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
  THE SOFTWARE.
*/
#ifndef X_FUTURE_H
#define X_FUTURE_H

#include "macros.h"
#include "splay.h"
#include "list.h"
#include "bitmap.h"
#include "cond.h"
#include "mutex.h"
#include "types.h"

struct x_fupool_st
{
	uint8_t bit_buf[8192];
	x_bitmap id_map;
	x_cond seq_cond;
	x_cond status_cond;
	x_mutex lock;
	x_splay fut_set;
	uint16_t last_id;
	uint16_t xxx;
};

#define X_FUTURE_NULL 0
#define X_FUTURE_PENDING 1
#define X_FUTURE_BUSY 2
#define X_FUTURE_READY 3

struct x_future_value_st
{
	void *data;
	int32_t retcode;
	uint32_t status;
};

struct x_future_st
{
	x_btnode node;
	x_link link;
	x_fupool *fupool;
	struct x_future_value_st value;
	uint16_t seq;
};

struct x_promise_st
{
	x_fupool *fupool;
	uint16_t seq;
	struct x_future_value_st *value;
};

void x_fupool_init(x_fupool *fup);
void x_fupool_free(x_fupool *fup);
uint16_t x_future_init(x_future *fut, x_fupool *fup, void *value);
bool x_future_is_ready(x_future *fut);
int x_future_wait(x_future *fut, int msec);
int x_future_wait_any(x_future **fut, size_t cnt, int msec);
int x_future_wait_all(x_future **fut, size_t cnt, int msec);
void x_future_free(x_future *fut);
void *x_promise_start(x_promise *prom, x_fupool *fup, uint16_t seq);
void x_promise_commit(x_promise *prom, int retcode);
uint16_t x_fupool_alloc_seq(x_fupool *fup);
void x_fupool_free_seq(x_fupool *fup, uint16_t seq);

#endif

