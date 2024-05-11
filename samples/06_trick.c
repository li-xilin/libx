#include "x/trick.h"
#include "x/narg.h"
#include "x/def.h"
#include <stdio.h>

#define show(line) printf("%s -> %s\n", #line, x_stringy(line))

int main()
{
	show(X_INC(3));
	show(X_DEC(3));

#define FUN1(i, x) fun1(i, x)
	show(X_TRANSFORM(FUN1, a, b, c, d, e, f));


#define FUN2(n, ...) fun2(n, __VA_ARGS__)
	show(X_PAVE_TO(3, FUN2, args...));

#define baseof_A B
#define baseof_B C
#define baseof_C D
#define baseof_D E
#define baseof_E F
	show(X_EXPAND_PREFIX(5, baseof_, A));

#define FUN3_1(a) fun3_1(a)
#define FUN3_2(a, b) fun3_2(a, b)
#define FUN3_3(a, b, c) fun3_3(a, b, c)
	show(X_OVERLOAD(FUN3_, 1));
	show(X_OVERLOAD(FUN3_, 1, 2));
	show(X_OVERLOAD(FUN3_, 1, 2, 3));

	show(X_CATENATE(_, a, b, c, d, e, f, g, h ,i));

	show(x_sizeof(char, short, int, long));

}
