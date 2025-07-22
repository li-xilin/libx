/*
 * Copyright (c) 2022-2025 Li Xilin <lixilin@gmx.com>
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

#ifndef X_SOCKMUX_H
#define X_SOCKMUX_H

#include "types.h"
#include "socket.h"
#include "mutex.h"

struct x_sockmux_st;

struct x_sockmux_ops_st {
	x_sockmux *(*m_create)(void);
	int (*m_add)(x_sockmux *mux, x_sock fd, short flags);
	int (*m_mod)(x_sockmux *mux, x_sock fd, short flags);
	void (*m_del)(x_sockmux *mux, x_sock fd, short flags);
	int (*m_poll)(x_sockmux *mux, struct timeval * timeout);
	void (*m_free)(x_sockmux *mux);
	x_sock (*m_next)(x_sockmux *mux, short *res_flags);
};

extern const struct x_sockmux_ops_st x_sockmux_epoll;

#endif
