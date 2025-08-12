#include "x/jpath.h"
#include "x/json.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// #include "x_json.h"
// #include "x_jpath.h"

static inline x_jpath *jsonpath_pop(x_jpath *jp);
static x_json *jsonpath_match_object(x_json *item, x_jpath *jp, bool case_sensitive, x_jpath_result *res);
static x_json *jsonpath_match_array(x_json *item, x_jpath *jp, x_jpath_result *res);
static void jsonpath_match_any(x_json *item, x_jpath *jp, bool case_sensitive, x_jpath_result *res, int mode);

static inline int skip_space(const char *data, int start);
static inline int get_end_of_key(const char *data, int start);
static x_jpath *jsonpath_parse(const char *data, size_t *pos);
static x_jpath *jsonpath_parse_key(const char *data, int *idx);
static x_jpath *jsonpath_parse_index(const char *data, int *idx);
static x_jpath *jsonpath_parse_any(const char *data, int *idx);

static bool jsonpath_match(x_json *item, x_jpath *jp, bool case_sensitive, x_jpath_result *res)
{
	int old_len = res->len;
	assert (item && jp);
	while (jp) {
		switch (jp->type) {
			case X_JPATH_START:
				if (!jp->next)
					x_jpath_result_add(res, item);
				jp = jp->next;
				break;
			case X_JPATH_KEY:
				item = jsonpath_match_object(item, jp, case_sensitive, res);
				jp = jp->next;
				break;
			case X_JPATH_INDEX:
				item = jsonpath_match_array(item, jp, res);
				jp = jp->next;
				break;
			case X_JPATH_ANYKEY:
				jsonpath_match_any(item, jp, case_sensitive, res, 1);
				jp = NULL;
				break;
			case X_JPATH_ANYINDEX:
				jsonpath_match_any(item, jp, case_sensitive, res, 2);
				jp = NULL;
				break;
			case X_JPATH_ANY:
				jsonpath_match(item, jp->next, case_sensitive, res);
				jsonpath_match_any(item, jp, case_sensitive, res, 3);
				jp = NULL;
				break;
			default:
				return false;
		}
	}
	return (res->len > old_len) ? true : false;
}

static void jsonpath_match_any(x_json *item, x_jpath *jp, bool case_sensitive, x_jpath_result *res, int mode)
{
	x_json *ele = NULL;
	bool is_last = (jp->next == NULL);
	if (mode == 1 && !x_json_is_object(item))
		return;
	else if (mode == 2 && !x_json_is_array(item))
		return;
	x_json_array_foreach(ele, item) {
		if (is_last && mode != 3) {
			x_jpath_result_add(res, ele);
			continue;
		}
		// jsonpath_match(ele, jp->next, case_sensitive, res);
		if (mode == 3) {
			jsonpath_match_any(ele, jp, case_sensitive, res, mode);
		}
	}
}

static x_json *jsonpath_match_object(x_json *item, x_jpath *jp, bool case_sensitive, x_jpath_result *res)
{
	bool is_last = false;
	if (!jp->next) {
		is_last = true;
	}
	x_json *tmp = NULL;
	tmp = x_json_object_at(item, jp->key, case_sensitive);
	if (is_last)
		x_jpath_result_add(res, tmp);
	return tmp;
}

static x_json *jsonpath_match_array(x_json *item, x_jpath *jp, x_jpath_result *res)
{
	bool is_last = false;
	if (!jp->next)
		is_last = true;
	x_json *tmp = NULL;
	if (x_json_is_array(item))
		tmp = x_json_array_at(item, jp->index);
	else {
		if (jp->index == 0) {
			tmp = item;
		}
	}
	if (is_last)
		x_jpath_result_add(res, tmp);
	return tmp;
}

static x_jpath *jsonpath_parse(const char *data, size_t *pos)
{
	x_jpath *jp = NULL;
	x_jpath *tmp = NULL;

	int i = skip_space(data, 0);
	if (data[i] && data[i++] == '$') {
		jp = x_jpath_create(X_JPATH_START);
		i = skip_space(data, i);
	} else {
		*pos = i;
		return NULL;
	}
	while (data[i] != '\0') {
		tmp = NULL;
		switch (data[i]) {
			case '.':
				tmp = jsonpath_parse_key(data, &i);
				break;
			case '[':
				tmp = jsonpath_parse_index(data, &i);
				break;
			case '*':
				tmp = jsonpath_parse_any(data, &i);
				break;
		}
		if (tmp)
			x_jpath_append(jp, tmp);
		else {
			*pos = i;
			x_jpath_free(jp);
			return NULL;
		}
		i = skip_space(data, i);
	}
	return jp;
}

static x_jpath *jsonpath_parse_any(const char *data, int *idx)
{
	(*idx)++; // advance past the first *
	x_jpath *item = NULL;
	// must followed by a *
	if (data[*idx] != '*')
		return NULL;
	(*idx)++;
	// can't be last and *
	if (data[*idx] == 0 || data[*idx] == '*')
		return NULL;
	item = x_jpath_create(X_JPATH_ANY);
	return item;
}

static x_jpath *jsonpath_parse_key(const char *data, int *idx)
{
	(*idx)++; // advance past the .
	x_jpath *item = NULL;
	*idx = skip_space(data, *idx);
	if (data[*idx] == '\0')
		return NULL;
	if (data[*idx] == '*') {
		*idx = *idx + 1;
		item = x_jpath_create(X_JPATH_ANYKEY);
		return item;
	}
	int start = *idx, end = get_end_of_key(data, start), len = 0;
	bool was_quoted = (data[start] == '"');
	*idx = end;
	if (was_quoted && data[end - 1] != '"')
		return NULL;
	if (was_quoted) {
		start++;
		end--;
	}
	len = end - start;
	if (len <= 0)
		return NULL;
	item = x_jpath_create(X_JPATH_KEY);
	item->key = (char*)x_malloc(NULL, sizeof(char) * (len + 1));
	strncpy(item->key, data + start, len);
	item->key[len] = '\0';
	return item;
}

static x_jpath *jsonpath_parse_index(const char *data, int *idx)
{
	(*idx)++; // advance past the [
	*idx = skip_space(data, *idx);
	x_jpath* item = NULL;
	if (data[*idx] == '*') {
		(*idx)++;
		item = x_jpath_create(X_JPATH_ANYINDEX);
	} else {
		int num_start = *idx;
		while (data[*idx] >= '0' && data[*idx] <= '9') {
			(*idx)++;
		}
		if (*idx == num_start) {
			return NULL;
		}
		long num = 0;
		num = strtol(data + num_start, NULL, 10);
		item = x_jpath_create(X_JPATH_INDEX);
		item->index = (int)num;
	}
	*idx = skip_space(data, *idx);
	if (data[*idx] != '\0' && data[(*idx)++] == ']') {
		return item;
	}
	x_jpath_free(item);
	return NULL;
}

static inline int skip_space(const char *data, int start)
{
	int i = start;
	while (*(data + i) && *(data + i) <= 32) {
		i++;
	}
	return i;
}

static inline int get_end_of_key(const char *data, int start)
{
	int idx = start;
	if (data[idx] == '"') {
		idx++; // advance past the opening "
		while (data[idx] != 0)
			switch (data[idx++]) {
				case '\\':
					/* Skip the next character after a backslash. It cannot mark
					   the end of the quoted string.  */
					idx++;
					break;
				case '"':
					return idx;
			}
		return idx;
	}
	while (data[idx] != '\0' && data[idx] != '*' && data[idx] != '.' && data[idx] != '[' && data[idx] > 32) {
		idx++;
	}
	return idx;
}

x_jpath *x_jpath_parse(const char *data, size_t *pos)
{
	return jsonpath_parse(data, pos);
}

bool x_jpath_match(x_json *item, x_jpath *head, bool case_sensitive, x_jpath_result *res)
{
	if ((head == NULL) || (item == NULL) || (head->type != X_JPATH_START)) {
		return false;
	}
	return jsonpath_match(item, head, case_sensitive, res);
}

x_jpath* x_jpath_create(enum x_jpath_type type)
{
	x_jpath* jp = (x_jpath*)x_malloc(NULL, sizeof(x_jpath));
	if (jp) {
		memset(jp, 0, sizeof(x_jpath));
	}
	jp->type = type;
	return jp;
}

void x_jpath_free(x_jpath *jp)
{
	x_jpath *next = NULL;
	while (jp) {
		next = jp->next;
		if (jp->key) {
			x_free(jp->key);
		}
		x_free(jp);
		jp = next;
	}
}

bool x_jpath_append(x_jpath *jp, x_jpath* item)
{
	if ((jp == NULL) || (item == NULL))
		return false;
	while (jp->next)
		jp = jp->next;
	jp->next = item;
	return true;
}

static void x_jpath_result_init(x_jpath_result *res)
{
	x_jpath_result_node* resnode = (x_jpath_result_node*)x_malloc(NULL, sizeof(x_jpath_result_node));
	resnode->next = NULL;
	resnode->node = NULL;
	res->head = resnode;
	res->len = 0;
}

x_jpath_result *x_jpath_result_create()
{
	x_jpath_result* res = (x_jpath_result*)x_malloc(NULL, sizeof(x_jpath_result));
	x_jpath_result_init(res);
	return res;
}

bool x_jpath_result_add(x_jpath_result *res, x_json *item)
{
	if (!item || !res)
		return false;
	x_jpath_result_node *tmp = (x_jpath_result_node*)x_malloc(NULL, sizeof(x_jpath_result_node));
	x_jpath_result_node *head = res->head;
	tmp->node = item;
	while (head->next) {
		head = head->next;
		if (head->node == item)
			return false;
	}
	tmp->next = head->next;
	head->next = tmp;
	res->len++;
	return true;
}
x_json *x_jpath_result_to_array(x_jpath_result *res)
{
	x_json *array = x_json_create_array();
	x_jpath_result_node *resnode = res->head->next;
	while (resnode) {
		x_json *item = x_json_copy(resnode->node, true);
		x_json_array_add(array, item);
		resnode = resnode->next;
	}
	return array;
}

void x_jpath_result_free(x_jpath_result *res)
{
	x_jpath_result_node *head = res->head;
	x_jpath_result_node *next = NULL;
	while (head) {
		next = head->next;
		x_free(head);
		head = next;
	}
	x_free(res);
}

bool x_jpath_wildcarded(x_jpath *jp)
{
	if (!jp)
		return false;
	while (jp) {
		if (jp->type == X_JPATH_ANY || jp->type == X_JPATH_ANYINDEX || jp->type == X_JPATH_ANYKEY)
			return true;
		jp = jp->next;
	}
	return false;
}

bool x_jpath_insert(x_json *root, x_jpath *jp, bool case_sensitive, x_json *value)
{
	if (!root || !jp || !value)
		return false;
	if (x_jpath_wildcarded(jp))
		return false; // can't use * and **
	x_jpath_result *res = x_jpath_result_create();
	if (x_jpath_match(root, jp, case_sensitive, res)) {
		x_jpath_result_free(res);
		return false; // Already exists
	}
	x_jpath *last = jsonpath_pop(jp);
	if (!x_jpath_match(root, jp, case_sensitive, res))
		return false;
	x_json *found = res->head->next->node;
	if (last->type == X_JPATH_INDEX) {
		if (x_json_is_array(found)) {
			x_json_array_add(found, value);
		}
		else if (last->index > 0) {
			// found a scalar or object and the index is not 0, auto wrap it
			x_json *n = x_json_copy(found, false);
			n->value.child = found->value.child;
			n->next = NULL;
			n->prev = NULL;
			found->type = X_JSON_ARRAY;
			found->value.child = NULL;
			x_json_array_add(found, n);
			x_json_array_add(found, value);
		}
		x_jpath_result_free(res);
		return true;
	}
	else if (last->type == X_JPATH_KEY && x_json_is_object(found)) {
		x_json_object_add(found, last->key, value);
		x_jpath_result_free(res);
		return true;
	}
	else {
		x_jpath_result_free(res);
		return false;
	}
}

static inline x_jpath *jsonpath_pop(x_jpath *jp)
{
	x_jpath *last = NULL;
	if (!jp)
		return NULL;
	while (jp->next && jp->next->next)
		jp = jp->next;
	last = jp->next;
	jp->next = NULL;
	return last;
}

