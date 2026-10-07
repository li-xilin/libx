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

#ifndef X_HFSM_H
#define X_HFSM_H

#include "linkage.h"
#include "types.h"

X_EXTERN_C_BEGIN

#define X_HFSM_INIT(cur_state, err_state) { NULL, cur_state, err_state }
#define X_HFSM_BUILTIN_TRANS 16

#define X_HFSM_STATE(_parent, _entry, _entry_action, _exit_action, _trans_list) { \
	.parent = _parent, \
	.entry = _entry, \
	.entry_action = _entry_action, \
	.exit_action = _exit_action, \
	.trans_list = _trans_list, \
}

#define X_HFSM_TRANS(type, internal, guard, action, next_state) { \
	.type = _type, \
	.internal = _internal, \
	.guard = _guard, \
	.action = _action, \
	.next_state = _next_state, \
}

struct x_hfsm_event_st
{
	int type;
};

struct x_hfsm_trans_st
{
	x_hfsm_state *next_state;
	int event_type;
	bool internal;
	bool (*guard)(x_hfsm_trans *trans, x_hfsm_event *event);
	void (*action)(x_hfsm_state *curr_state, x_hfsm_state *new_state, x_hfsm_event *event);
};

typedef void x_hfsm_action(x_hfsm_state *state, x_hfsm_event *event);

struct x_hfsm_state_st
{
	x_hfsm_state *parent, *entry;
	x_hfsm_action *entry_action, *exit_action;
	x_hfsm_trans *trans_list[X_HFSM_BUILTIN_TRANS];
};

struct x_hfsm_st
{
	x_hfsm_state *prev_state;
	x_hfsm_state *curr_state;
	x_hfsm_state *err_state;
};

enum x_hfsm_status
{
	X_HFSM_ERROR,
	X_HFSM_CHANGED,
	X_HFSM_SELFLOOP,
	X_HFSM_NOMATCH,
	X_HFSM_FINISHED,
};

static inline void x_hfsm_init(x_hfsm *fsm, x_hfsm_state *initial_state, x_hfsm_state *err_state)
{
	fsm->prev_state = NULL;
	fsm->curr_state = initial_state;
	fsm->err_state = err_state;
}

static inline bool x_hfsm_stopped(x_hfsm *fsm)
{
	if (!fsm)
		return true;
	return !fsm->curr_state->trans_list[0];
}

enum x_hfsm_status x_hfsm_handle_event(x_hfsm *fsm, x_hfsm_event *event);

X_EXTERN_C_END

#endif

