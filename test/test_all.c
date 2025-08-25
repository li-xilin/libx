#include "x/test.h"
#include "x/flowctl.h"
#include <stdio.h>
#include <stdlib.h>

void process(const char *suite_name, const char *case_name, int pos, int total)
{
	fprintf(stderr, "[%d/%d]", pos, total);
}

void output(const char* suite_name, ut_case *tc, void *arg)
{
	ut_case_dump_file(suite_name, tc, arg);
}

void regex_test_init(ut_suite *s);

int main(int argc, char *argv[])
{
	ut_runner r;
	ut_runner_init(&r, output, stderr);

	ut_suite regex_test;
	regex_test_init(&regex_test);
	ut_runner_add(&r, &regex_test);

	ut_runner_run(&r, process);
}

