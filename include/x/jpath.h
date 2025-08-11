#ifndef X_JPATH__H
#define X_JPATH__H

#include "types.h"
#include "json.h"
#include "memory.h"
#include "flowctl.h"
#include <stdbool.h>

enum x_jpath_type {
    X_JPATH_INVALID = 0,
    X_JPATH_START,
    X_JPATH_KEY,
    X_JPATH_INDEX,
    X_JPATH_ANYKEY,
    X_JPATH_ANYINDEX,
    X_JPATH_ANY,
};

struct x_jpath_st
{
    x_jpath *next;
    enum x_jpath_type type;
    char *key;
    int index;
};

struct x_jpath_result_node_st
{
    x_jpath_result_node *next;
    x_json *node;
};

struct x_jpath_result_st
{
    x_jpath_result_node *head;
    int len;
};

x_jpath *x_jpath_parse(const char *data, size_t *pos);
bool x_jpath_wildcarded(x_jpath *jp);
bool x_jpath_match(x_json* item, x_jpath* jp, bool case_sensitive, x_jpath_result* res);
bool x_jpath_insert(x_json* root, x_jpath* jp, bool case_sensitive, x_json* value);

x_jpath *x_jpath_create(enum x_jpath_type type);
void x_jpath_free(x_jpath *head);
bool x_jpath_append(x_jpath *jp, x_jpath *item);

x_jpath_result *x_jpath_result_create(void);
void x_jpath_result_free(x_jpath_result *res);
x_json *x_jpath_result_to_array(x_jpath_result *res);
bool x_jpath_result_add(x_jpath_result *res, x_json *item);

#define x_jpath_result_foreach(i, res) \
	for (x_jpath_result_node *__x_jpath_cur = (res)->head->next; __x_jpath_cur; __x_jpath_cur = __x_jpath_cur->next) \
		x_block_var(x_json *i = __x_jpath_cur->node)

#endif

