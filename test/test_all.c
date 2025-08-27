#include "x/test.h"
#include "x/flowctl.h"
#include <stdio.h>
#include <stdlib.h>

#define ADD_SUITE(name) \
	void name##_init(ut_suite *s); \
	ut_suite name; \
	name##_init(&name); \
	ut_runner_add(&r, &name)

void process(const char *suite_name, const char *case_name, int pos, int total)
{
	fprintf(stderr, "[%d/%d]", pos, total);
}

void output(const char* suite_name, ut_case *tc, void *arg)
{
	ut_case_dump_file(suite_name, tc, arg);
}

int main(int argc, char *argv[])
{
	ut_runner r;
	ut_runner_init(&r, output, stderr);

	ADD_SUITE(regex_test);
	ADD_SUITE(future_test);

	ut_runner_run(&r, process);
}

