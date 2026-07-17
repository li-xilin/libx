#include "x/hfsm.h"
#include "x/test.h"
#include <unistd.h>
#include <stdio.h>
#include <time.h>
#include <string.h>
#include <stdlib.h>

/* This simple test uses a multiple-nested state machine to test that
 * traversing parents and children works correctly.
 *
 *     .--.         o
 *     |  v         |
 * .---|------[9]---|----------.
 * |   |            v          |
 * |   |   o      .---.   (b)  |  .---.
 * |   |   |      | 1 |<----------| 2 |<---.
 * |   |   |      `---`        |  `---`<-. |
 * |   |   |        |(d)       |         | |
 * | .-|---|--[10]--|--------. |         | |
 * | | |   |        v        | |      (f)| |(g)
 * | | o   |      .---.<----------.      | |
 * | |     |      | 3 |-.    | |  |(a)   | |
 * | |     |      `---` |(e) | |--`      | |
 * | |     |            v    | |         | |
 * | | .---|----[11]-------. |-----------' |
 * | | |   v             o |---------------'
 * | | | .---. (h).---.  | | | |
 * | | | | 4 |--->| 5 |<-' | | |
 * | | | `---'    `---'    | | |
 * | | |   |(j)     |      | | |
 * | | |   |        |      | | |      .---.    
 * | | `---|--------|------' |------> | 6 |   
 * | `-----|--------|--------' |  (i) `---'       
 * `-------|-----^--|----------'        .---.
 *      ^  |     |  |                   | E |
 *      `--'     `--'                   `---'
 */

enum {
	EVENT_DUMMY,
};

struct event {
	x_hfsm_event e;
	char data;
	const char *expected_state;
	ut_runner *runner;
};

struct state {
	x_hfsm_state s;
	char str[16];
};

struct trans {
	x_hfsm_trans t;
	char cond;
};

static void entry_action(x_hfsm_state *state, x_hfsm_event *event);
static void exit_action(x_hfsm_state *state, x_hfsm_event *event);
static void trans_action(x_hfsm_state *old_state, x_hfsm_state *new_state ,x_hfsm_event *event);
static bool guard(x_hfsm_trans *trans, x_hfsm_event *event);

static struct state s1, s2, s3, s4, s5, s6, s9, s10, s11, serr;

static struct trans
	s1_to_s3 = { { &s3.s, EVENT_DUMMY, false, guard, &trans_action }, 'd' },
	s2_to_s1 = { { &s1.s, EVENT_DUMMY, false, guard, &trans_action }, 'b' },
	s3_to_s11 = { { &s11.s, EVENT_DUMMY, false, guard, &trans_action }, 'e' },
	s4_to_s5 = { { &s5.s, EVENT_DUMMY, false, guard, &trans_action }, 'h' },
	s4_to_s9 = { { &s9.s, EVENT_DUMMY, false, guard, &trans_action }, 'j' },
	s5_to_s10 = { { &s10.s, EVENT_DUMMY, false, NULL, &trans_action }},
	s9_to_s3 = { { &s3.s, EVENT_DUMMY, false, guard, &trans_action }, 'a'},
	s10_to_s2 = { { &s2.s, EVENT_DUMMY, false, guard, &trans_action }, 'f'},
	s10_to_s6 = { { &s6.s, EVENT_DUMMY, false, guard, &trans_action }, 'i'},
	s11_to_s2 = { { &s2.s, EVENT_DUMMY, false, guard, &trans_action }, 'g'};

static struct state
	s1 = { { &s9.s, NULL, entry_action, exit_action, { &s1_to_s3.t } }, "1" },
	s2 = { { NULL, NULL, entry_action, exit_action, { &s2_to_s1.t } }, "2" },
	s3 = { { &s10.s, NULL, entry_action, exit_action, { &s3_to_s11.t } }, "3" },
	s4 = { { &s11.s, NULL, entry_action, exit_action, { &s4_to_s5.t, &s4_to_s9.t } }, "4" },
	s5 = { { &s11.s, NULL, entry_action, exit_action, { &s5_to_s10.t } }, "5" },
	s6 = { { NULL, NULL, entry_action, exit_action, { NULL } }, "6" },
	s9 = { { NULL, &s4.s, entry_action, exit_action, { &s9_to_s3.t } }, "9" },
	s10 = { { &s9.s, &s9.s, entry_action, exit_action, { &s10_to_s2.t, &s10_to_s6.t} }, "10" },
	s11 = { { &s10.s, &s5.s, entry_action, exit_action, { &s11_to_s2.t } }, "11" },
	serr = { { &s9.s, NULL, entry_action, NULL, { NULL } }, "err" };

static void entry_action(x_hfsm_state *state, x_hfsm_event *event)
{
}

static void exit_action(x_hfsm_state *state, x_hfsm_event *event)
{
}

static void trans_action(x_hfsm_state *old_state, x_hfsm_state *new_state, x_hfsm_event *event)
{
	struct event *e = x_container_of(event, struct event, e);
	struct state *new_s = x_container_of(new_state, struct state, s);
	ut_assert_str_equal(e->runner, e->expected_state, new_s->str);
}

static bool guard(x_hfsm_trans *trans, x_hfsm_event *event)
{
	struct event *e = x_container_of(event, struct event, e);
	struct trans *t = x_container_of(trans, struct trans, t);
	return t->cond == e->data;
}

static void multiple_nested(ut_runner *r)
{
	int res, i;
	x_hfsm fsm;
	x_hfsm_init(&fsm, &s1.s, &serr.s);
	struct event events[] = {
		{ { EVENT_DUMMY }, 'd', "3"},
		{ { EVENT_DUMMY }, 'e', "5"},
		{ { EVENT_DUMMY }, '*', "4"},
		{ { EVENT_DUMMY }, 'j', "4"},
		{ { EVENT_DUMMY }, 'g', "2"},
		{ { EVENT_DUMMY }, 'b', "1"},
		{ { EVENT_DUMMY }, 'd', "3"},
		{ { EVENT_DUMMY }, 'e', "5"},
		{ { EVENT_DUMMY }, 'k', "4"},
		{ { EVENT_DUMMY }, 'h', "5"},
		{ { EVENT_DUMMY }, '*', "4"},
		{ { EVENT_DUMMY }, 'f', "2"},
		{ { EVENT_DUMMY }, 'b', "1"},
		{ { EVENT_DUMMY }, 'a', "3"},
		{ { EVENT_DUMMY }, 'f', "2"},
		{ { EVENT_DUMMY }, 'b', "1"},
		{ { EVENT_DUMMY }, 'd', "3"},
		{ { EVENT_DUMMY }, 'i', "6"},
	};
	for (i = 0; i < x_arrlen(events) - 1; ++i) {
		events[i].runner = r;
		res = x_hfsm_handle_event(&fsm, &events[i].e);
		if (res == X_HFSM_SELFLOOP) {
			ut_assert(r, i != 0);
			ut_assert_str_equal(r, events[i].expected_state, events[i - 1].expected_state);
		}
		else {
			ut_assert(r, res == X_HFSM_CHANGED);
		}
	}
	events[i].runner = r;
	res = x_hfsm_handle_event(&fsm, &events[i].e);
	ut_assert_uint_equal(r, X_HFSM_FINISHED, res);
	res = x_hfsm_handle_event(&fsm, &events[0].e);
	ut_assert_uint_equal(r, X_HFSM_ERROR, res);
}

void hfsm_test_init(ut_suite *s)
{
	ut_suite_init(s, "hfsm.h");
	ut_suite_add(s, multiple_nested);
}

