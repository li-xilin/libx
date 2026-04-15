#include <x/except.h>
#include <stdio.h>

int init1(void)
{
	printf("%s\n", __func__);
	return 0;
}

int init2(void)
{
	printf("%s\n", __func__);
	return 0;
}

int init3(void)
{
	printf("%s\n", __func__);
	return -1; // ERROR returned
}

void clean1(void)
{
	printf("%s\n", __func__);
}

void clean2(void)
{
	printf("%s\n", __func__);
}

void clean3(void)
{
	printf("%s\n", __func__);
}

int main(void)
{
	x_finish {
		printf("finished\n");
		return 1;
	}

	if (init1())
		x_throw;
	x_catch (1)
		clean1();

	if (init2())
		x_throw;
	x_catch (2)
		clean2();

	if (init3())
		x_throw;
	x_catch (3)
		clean3();
}

