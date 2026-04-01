#include "x/flowctl.h"
#include "x/test.h"
#include "x/pipe.h"
#include <unistd.h>
#include <stdio.h>
#include <time.h>
#include <string.h>

#define assert_equal(pipe, data, size) \
{ \
	char tmp[32]; \
	size_t l = x_pipe_read(pipe, tmp, 32); \
	ut_assert_mem_equal(r, data, size, tmp, l); \
}

static void resize(ut_runner *r)
{
	x_block {
		unsigned char buf[6] = { 'a', 'b', 'c', 0, 0, 0 };
		x_pipe pipe = { .buf = buf, .front = 0, .rear = 3, .size = 5 };
		x_pipe_resize(&pipe, 6);
		assert_equal(&pipe, "abc", 3);
	}

	x_block {
		unsigned char buf[6] = { 0, 'a', 'b', 'c', 0, 0 };
		x_pipe pipe = { .buf = buf, .front = 1, .rear = 4, .size = 5 };
		x_pipe_resize(&pipe, 6);
		assert_equal(&pipe, "abc", 3);
	}

	x_block {
		unsigned char buf[6] = { 0, 0, 'a', 'b', 'c', 0 };
		x_pipe pipe = { .buf = buf, .front = 2, .rear = 0, .size = 5 };
		x_pipe_resize(&pipe, 6);
		assert_equal(&pipe, "abc", 3);
	}

	x_block {
		unsigned char buf[6] = { 'c', 0, 0, 'a', 'b', 0 };
		x_pipe pipe = { .buf = buf, .front = 3, .rear = 1, .size = 5 };
		x_pipe_resize(&pipe, 6);
		assert_equal(&pipe, "abc", 3);
	}

	x_block {
		unsigned char buf[6] = { 'b', 'c', 0, 0, 'a', 0 };
		x_pipe pipe = { .buf = buf, .front = 4, .rear = 2, .size = 5 };
		x_pipe_resize(&pipe, 6);
		assert_equal(&pipe, "abc", 3);
	}
}

void pipe_test_init(ut_suite *s)
{
	ut_suite_init(s, "pipe.h");
	ut_suite_add(s, resize);
}

