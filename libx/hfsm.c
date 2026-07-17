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

#include "x/hfsm.h"
#include "x/macros.h"
#include <assert.h>

static void goto_err_state(x_hfsm *fsm, x_hfsm_event *event)
{
	fsm->prev_state = fsm->curr_state;
	fsm->curr_state = fsm->err_state;
	if (fsm->curr_state && fsm->curr_state->entry_action)
		fsm->curr_state->entry_action(fsm->curr_state, event);
}

static x_hfsm_trans *match_trans(x_hfsm *fsm, x_hfsm_state *state, x_hfsm_event *event)
{
	x_hfsm_trans *t;
	for (size_t i = 0; (t = state->trans_list[i]); i++) {
		if (t->event_type != event->type)
			continue;
		if (!t->guard || t->guard(t, event))
			return t;
	}
	return NULL;
}

enum x_hfsm_status x_hfsm_handle_event(x_hfsm *fsm, x_hfsm_event *event)
{
	assert(fsm && event);
	if (!fsm->curr_state) {
		goto_err_state(fsm, event);
		return X_HFSM_ERROR;
	}
	if (!fsm->curr_state->trans_list[0])
		return X_HFSM_ERROR;
	x_hfsm_state *curr_state = fsm->curr_state;
	do {
		x_hfsm_trans *trans = match_trans(fsm, curr_state, event);
		if (!trans) {
			curr_state = curr_state->parent;
			continue;
		}
		if (!trans->next_state) {
			goto_err_state(fsm, event);
			return X_HFSM_ERROR;
		}
		curr_state = trans->next_state;
		while (curr_state->entry)
			curr_state = curr_state->entry;
		if (!trans->internal && fsm->curr_state->exit_action)
			fsm->curr_state->exit_action(fsm->curr_state, event);
		if (trans->action)
			trans->action(fsm->curr_state, curr_state, event);
		if (!trans->internal && curr_state->entry_action)
			curr_state->entry_action(curr_state, event);
		fsm->prev_state = fsm->curr_state;
		fsm->curr_state = curr_state;
		if (fsm->curr_state == fsm->prev_state)
			return X_HFSM_SELFLOOP;
		if (fsm->curr_state == fsm->err_state)
			return X_HFSM_ERROR;
		if (!fsm->curr_state->trans_list[0])
			return X_HFSM_FINISHED;
		return X_HFSM_CHANGED;
	} while (curr_state);
	return X_HFSM_NOMATCH;
}

