#ifndef X_REGEX_H
#define X_REGEX_H

#include "types.h"

#define X_REGEX_CATCH_MAX 16

struct x_recatch_st {
	struct {
		const char *str;
		int pos, len;
	} text;
	struct x_recatch_item_st {
		const char *ptr;
		int len;
		int id;
	} items [X_REGEX_CATCH_MAX];
	int idx, count;
};

int x_rematch(const char *txt, const char *re, x_recatch *c);
int x_recatch_size(x_recatch *c);
const char *x_recatch_at(x_recatch *c, int index);
int x_recatch_len_at(x_recatch *c, int index);
char *x_recatch_fetch(x_recatch *c, char *str, int index);
char *x_recatch_replace(x_recatch *c, char *new_str, const char *rpl_str, int id);
char *x_recatch_put(x_recatch *c, char *new_str, const char *put_str);

#endif


