#include "x/test.h"
#include "x/flowctl.h"
#include <stdio.h>
#include <stdlib.h>

void suite1_case1(ut_runner *r)
{
	ut_assert_str_equal(r, "234", "123");
}

void suite1_case2(ut_runner *r)
{
	ut_assert_mem_equal(r, "hello", 5, "holle", 5);
}

void suite2_case1(ut_runner *r)
{
	ut_assert_int_equal(r, 1, 1);
}

void suite2_case2(ut_runner *r)
{
	ut_printf(r, "This is a notice message");
}

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

	ut_suite s1;
	ut_suite_init(&s1, "suite1");
	ut_suite_add(&s1, suite1_case1);
	ut_suite_add(&s1, suite1_case2);

	ut_suite s2;
	ut_suite_init(&s2, "suite2");
	ut_suite_add(&s2, suite2_case1);
	ut_suite_add(&s2, suite2_case2);

	ut_runner_add(&r, &s1);
	ut_runner_add(&r, &s2);

	ut_runner_run(&r, process);
}

