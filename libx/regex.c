#include "x/unicode.h"
#include "x/regex.h"
#include "x/string.h"
#include <ctype.h>
#include <wchar.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>

#define UTF8_BIT (uint8_t)0x80
#define INF 1073741824 // 2^30

#define MOD_ALPHA 1
#define MOD_OMEGA 2
#define MOD_LONLEY 4
#define MOD_FWRBYCHAR 8
#define MOD_COMMUNISM 16
#define MOD_NEGATIVE 128

enum re_type
{
	PATH,
	GROUP,
	HOOK,
	SET,
	BACKREF,
	META,
	RANGEAB,
	UTF8,
	POINT,
	SIMPLE
};

struct pattern
{
	const char *ptr;
	enum re_type type;
	unsigned int loops_min, loops_max;
	int len;
	uint32_t mods;
};

static int walker(struct pattern rexp, x_recatch *catch);
static int trekking(struct pattern *rexp , x_recatch *catch);
static int looper (struct pattern *rexp, x_recatch *catch);
static int loop_group(struct pattern *rexp, x_recatch *catch);
static int tracker (struct pattern *rexp, struct pattern *track);
static int tracker_set (struct pattern *rexp, struct pattern *track);
static void cut_simple (struct pattern *rexp, struct pattern *track);
static int cut_by_type (struct pattern *rexp, struct pattern *track, int type);
static void cut_by_len (struct pattern *rexp, struct pattern *track, int len, int type);
static void cut_regexp (struct pattern *rexp, int len);
static int walk_meta (const char *str, int len);
static int walk_set (const char *str, int len);
static void get_mods (struct pattern *rexp, struct pattern *track);
static void get_loops (struct pattern *rexp, struct pattern *track);
static int match (struct pattern *rexp, x_recatch *catch);
static int match_set (struct pattern rexp, x_recatch *catch);
static int match_back_ref (struct pattern *rexp, x_recatch *catch);
static int match_range (struct pattern *rexp, int chr);
static int match_meta (struct pattern *rexp, const char *txt);
static int match_text (struct pattern *rexp, const char *txt);
static void open_catch(x_recatch *catch, int *index);
static void close_catch(x_recatch *catch, int index);
static int last_id_catch(x_recatch *catch, int id);
static int digit_count(const char *str);

static int digit_count(const char *str)
{
	for(int digits = 0; ; digits++)
		if (isdigit(*str++) == 0)
			return digits;
}

int x_rematch(const char *txt, const char *re, x_recatch *catch)
{
	struct pattern rexp = {
		.ptr = re,
		.type = PATH,
		.len = strlen(re),
		.mods = 0,
	};
	int result = 0;
	catch->text.len = strlen(txt);
	catch->items[0].ptr = txt;
	catch->items[0].len = catch->text.len;
	catch->items[0].id = 0;
	catch->count = 1;
	if (catch->text.len == 0 || rexp.len == 0)
		return 0;
	get_mods(&rexp, &rexp);
	for(int forward, i = 0, loops = rexp.mods & MOD_ALPHA ? 1: catch->text.len;
			i < loops; i += forward) {
		forward = x_utf8_meter(txt + i);
		catch->idx = 1;
		catch->text.pos = 0;
		catch->text.str = txt + i;
		catch->text.len = catch->items[0].len - i;
		if (walker(rexp, catch)) {
			if (rexp.mods & MOD_OMEGA) {
				if (catch->text.pos == catch->text.len)
					return true;
				else
					catch->count = 1;
			}
			else if (rexp.mods & MOD_LONLEY)
				return true;
			else if ((rexp.mods & MOD_FWRBYCHAR) || catch->text.pos == 0)
				result++;
			else {
				forward = catch->text.pos;
				result++;
			}
		}
	}
	return result;
}

static int walker(struct pattern rexp, x_recatch *catch) {

	struct pattern track;
	for(int tpos = catch->text.pos, cindex = catch->count, cidx = catch->idx;
			cut_by_type(&rexp, &track, PATH);
			catch->text.pos = tpos, catch->count = cindex, catch->idx = cidx)
		if (trekking(&track, catch))
			return true;
	return false;
}

static int trekking(struct pattern *rexp , x_recatch *catch)
{
	struct pattern track;
	int catch_id;
	for(int result = false; tracker(rexp, &track); ) {
		switch (track.type) {
			case HOOK:
				open_catch(catch, &catch_id);
				result = loop_group(&track, catch);
				if (result)
					close_catch(catch, catch_id);
				break;
			case GROUP: case PATH:
				result = loop_group(&track, catch);
				break;
			case SET:
				if (track.ptr[0] == '^') {
					cut_regexp(&track, 1);
					track.mods |= MOD_NEGATIVE;
				}
			case BACKREF: case META: case RANGEAB: case UTF8: case POINT: case SIMPLE:
				result = looper(&track, catch);
		}
		if (result == false)
			return false;
	}
	return true;
}

static int looper (struct pattern *rexp, x_recatch *catch)
{
	int forward, loops = 0;
	while (loops < rexp->loops_max && catch->text.pos < catch->text.len &&
			(forward = match(rexp, catch))) {
		catch->text.pos += forward;
		loops++;
	}
	return loops < rexp->loops_min ? false: true;
}

static int loop_group(struct pattern *rexp, x_recatch *catch)
{
	int loops = 0;
	while (loops < rexp->loops_max && walker(*rexp, catch))
		loops++;
	return loops < rexp->loops_min ? false: true;
}

static int tracker(struct pattern *rexp, struct pattern *track) {

	if (rexp->len == 0)
		return false;
	switch (*rexp->ptr & UTF8_BIT ? UTF8: *rexp->ptr) {
		case ':':
			cut_by_len (rexp, track, 2, META);
			break;
		case '.':
			cut_by_len (rexp, track, 1, POINT);
			break;
		case '@':
			cut_by_len (rexp, track, 1 + digit_count(rexp->ptr + 1), BACKREF);
			break;
		case '(':
			cut_by_type(rexp, track, GROUP);
			break;
		case '<':
			cut_by_type(rexp, track, HOOK);
			break;
		case '[':
			cut_by_type(rexp, track, SET);
			break;
		case UTF8:
			cut_by_len (rexp, track, x_utf8_meter(rexp->ptr), UTF8);
			break;
		default:
			cut_simple(rexp, track);
			break;
	}
	get_loops(rexp, track);
	get_mods (rexp, track);
	return true;
}

static void cut_simple(struct pattern *rexp, struct pattern *track)
{
	for(int i = 1; i < rexp->len; i++)
		switch (rexp->ptr[i] & UTF8_BIT ? UTF8: rexp->ptr[i]) {
			case '(': case '<': case '[': case '@': case ':': case '.': case UTF8:
				cut_by_len(rexp, track, i, SIMPLE);
				return;
			case '?': case '+': case '*': case '{': case '#':
				if (i == 1)
					cut_by_len(rexp, track, 1, SIMPLE);
				else
					cut_by_len(rexp, track, i - 1, SIMPLE);
				return;
		}
	cut_by_len(rexp, track, rexp->len, SIMPLE);
}

static void cut_by_len(struct pattern *rexp, struct pattern *track, int len, int type)
{
	*track = *rexp;
	track->type = type;
	track->len = len;
	cut_regexp(rexp, len);
}

static int cut_by_type(struct pattern *rexp, struct pattern *track, int type)
{
	if (rexp->len == 0)
		return false;
	*track = *rexp;
	track->type = type;
	for(int cut, i = 0, deep = 0; (i += walk_meta(rexp->ptr + i, rexp->len - i)) < rexp->len; i++) {
		switch (rexp->ptr[i]) {
			case '(': case '<':
				deep++;
				break;
			case ')': case '>':
				deep--;
				break;
			case '[':
				i += walk_set(rexp->ptr + i, rexp->len - i);
				break;
		}
		switch (type) {
			case HOOK:
				cut = deep == 0;
				break;
			case GROUP:
				cut = deep == 0;
				break;
			case SET:
				cut = rexp->ptr[i] == ']';
				break;
			case PATH:
				cut = deep == 0 && rexp->ptr[i] == '|';
				break;
		}
		if (cut) {
			track->len = i;
			cut_regexp(rexp, i + 1);
			if (type != PATH)
				cut_regexp(track, 1);
			return true;
		}
	}
	cut_regexp(rexp, rexp->len);
	return true;
}

static void cut_regexp(struct pattern *rexp, int len)
{
	rexp->ptr += len;
	rexp->len -= len;
}

static int walk_set(const char *str, int len)
{
	for(int i = 0; (i += walk_meta(str + i, len - i)) < len; i++)
		if (str[i] == ']')
			return i;
	return len;
}

static int walk_meta(const char *str, int len)
{

	for(int i = 0;
			i < len;
			i += 2)
		if (str[i] != ':')
			return i;
	return len;
}

static void get_mods(struct pattern *rexp, struct pattern *track)
{
	int inMods = *rexp->ptr == '#', pos = 0;
	while (inMods)
		switch (rexp->ptr[++pos]) {
			case '^':
				track->mods |= MOD_ALPHA ;
				break;
			case '$':
				track->mods |= MOD_OMEGA ;
				break;
			case '?':
				track->mods |= MOD_LONLEY ;
				break;
			case '~':
				track->mods |= MOD_FWRBYCHAR ;
				break;
			case '*':
				track->mods |= MOD_COMMUNISM ;
				break;
			case '/':
				track->mods &= ~MOD_COMMUNISM ;
				break;
			default:
				inMods = false ;
				break;
		}
	cut_regexp(rexp, pos);
}

static void get_loops(struct pattern *rexp, struct pattern *track)
{
	track->loops_min = 1;
	track->loops_max = 1;
	if (!rexp->len)
		return;
	switch (*rexp->ptr) {
		case '?':
			cut_regexp(rexp, 1);
			track->loops_min = 0;
			track->loops_max = 1;
			return;
		case '+':
			cut_regexp(rexp, 1);
			track->loops_min = 1;
			track->loops_max = INF;
			return;
		case '*':
			cut_regexp(rexp, 1);
			track->loops_min = 0;
			track->loops_max = INF;
			return;
		case '{':
			cut_regexp(rexp, 1);
			track->loops_min = atoi(rexp->ptr);
			cut_regexp(rexp, digit_count(rexp->ptr));
			if (*rexp->ptr == ',') {
				cut_regexp(rexp, 1);
				if (*rexp->ptr == '}')
					track->loops_max = INF;
				else {
					track->loops_max = atoi(rexp->ptr);
					cut_regexp(rexp, digit_count(rexp->ptr));
				}
				cut_regexp(rexp, 1);
				break;
			}
			track->loops_max = track->loops_min;
			cut_regexp(rexp, 1);
			break;
	}
}

static int match (struct pattern *rexp, x_recatch *catch)
{
	switch (rexp->type) {
		case POINT:
			return x_utf8_meter(catch->text.str + catch->text.pos);
		case SET:
			return match_set (*rexp, catch);
		case BACKREF:
			return match_back_ref(rexp, catch);
		case RANGEAB:
			return match_range(rexp, catch->text.str[catch->text.pos]);
		case META:
			return match_meta (rexp, catch->text.str + catch->text.pos);
		default:
			return match_text (rexp, catch->text.str + catch->text.pos);
	}
}

static int match_text(struct pattern *rexp, const char *txt) {

	if (rexp->mods & MOD_COMMUNISM)
		return x_strnicmp(txt, rexp->ptr, rexp->len) == 0 ? rexp->len: 0;
	else
		return strncmp(txt, rexp->ptr, rexp->len) == 0 ? rexp->len: 0;
}

static int match_range(struct pattern *rexp, int chr) {

	if (rexp->mods & MOD_COMMUNISM) {
		chr = tolower(chr);
		return chr >= tolower(rexp->ptr[0]) && chr <= tolower(rexp->ptr[2]);
	} else
		return chr >= rexp->ptr[0] && chr <= rexp->ptr[2];
}

static int match_meta(struct pattern *rexp, const char *txt) {

	switch (rexp->ptr[1]) {
		case 'a':
			return !!x_isalpha_7bit(*txt);
		case 'A':
			return !x_isalpha_7bit(*txt) ? x_utf8_meter(txt): false;
		case 'd':
			return !!x_isdigit_7bit(*txt);
		case 'D':
			return !x_isdigit_7bit(*txt) ? x_utf8_meter(txt): false;
		case 'w':
			return !!x_isalnum_7bit(*txt);
		case 'W':
			return !x_isalnum_7bit(*txt) ? x_utf8_meter(txt): false;
		case 's':
			return !!x_isspace_7bit(*txt);
		case 'S':
			return !x_isspace_7bit(*txt) ? x_utf8_meter(txt): false;
		case '&':
			return *txt & UTF8_BIT ? x_utf8_meter(txt): false;
		default:
			return *txt == rexp->ptr[1];
	}
}

static int match_set (struct pattern rexp, x_recatch *catch)
{
	struct pattern track;
	for(int result = 0; tracker_set(&rexp, &track);) {
		switch (track.type) {
			case RANGEAB: case META: case UTF8:
				result = match(&track, catch);
				break;
			default:
				if (track.mods & MOD_COMMUNISM)
					result = x_strnichr(track.ptr, catch->text.str[catch->text.pos], track.len) != 0;
				else
					result = x_strnchr (track.ptr, catch->text.str[catch->text.pos], track.len) != 0;
		}
		if (result)
			return rexp.mods & MOD_NEGATIVE ? false: result;
	}
	return rexp.mods & MOD_NEGATIVE ? x_utf8_meter(catch->text.str + catch->text.pos): false;
}

static int tracker_set(struct pattern *rexp, struct pattern *track) {

	if (rexp->len == 0)
		return false;
	switch (*rexp->ptr & UTF8_BIT ? UTF8: *rexp->ptr) {
		case ':':
			cut_by_len(rexp, track, 2, META);
			break;
		case UTF8:
			cut_by_len(rexp, track, x_utf8_meter(rexp->ptr), UTF8);
			break;
		default:
			for(int i = 1; i < rexp->len; i++)
				switch (rexp->ptr[i] & UTF8_BIT ? UTF8: rexp->ptr[i]) {
					case ':': case UTF8:
						cut_by_len(rexp, track, i, SIMPLE);
						goto set_lm;
					case '-':
						if (i == 1) cut_by_len(rexp, track, 3, RANGEAB);
						else cut_by_len(rexp, track, i - 1, SIMPLE);
						goto set_lm;
				}
			cut_by_len(rexp, track, rexp->len, SIMPLE);
	}
set_lm:
	track->loops_min = track->loops_max = 1;
	return true;
}

static int match_back_ref (struct pattern *rexp, x_recatch *catch)
{
	int backRefId = atoi(rexp->ptr + 1);
	int backRefIndex = last_id_catch(catch, backRefId);
	if (x_recatch_at(catch, backRefIndex) == 0 || strncmp(catch->text.str + catch->text.pos,
				x_recatch_at(catch, backRefIndex), x_recatch_len_at(catch, backRefIndex)) != 0)
		return false;
	else
		return x_recatch_len_at(catch, backRefIndex);
}

static int last_id_catch(x_recatch *catch, int id)
{
	for(int index = catch->count - 1; index > 0; index--)
		if (catch->items[index].id == id)
			return index;
	return X_REGEX_CATCH_MAX;
}

static void open_catch(x_recatch *catch, int *index)
{
	if (catch->count >= X_REGEX_CATCH_MAX) {
		*index = X_REGEX_CATCH_MAX;
		return;
	}
	*index = catch->count;
	catch->items[*index].ptr = catch->text.str + catch->text.pos;
	catch->items[*index].id = catch->idx++;
	catch->count++;
}

static void close_catch(x_recatch *catch, int index)
{
	if (index < X_REGEX_CATCH_MAX)
		catch->items[index].len = &catch->text.str[catch->text.pos] - catch->items[index].ptr;
}

int x_recatch_size(x_recatch *catch)
{
	return catch->count - 1;
}

const char *x_recatch_at(x_recatch *catch, int index)
{
	return (index > 0 && index < catch->count) ? catch->items[index].ptr: 0;
}

int x_recatch_len_at(x_recatch *catch, int index)
{
	return (index > 0 && index < catch->count) ? catch->items[index].len: 0;
}

char *x_recatch_fetch(x_recatch *catch, char *str, int index)
{
	if (index > 0 && index < catch->count) {
		int len = catch->items[index].len;
		strncpy(str, catch->items[index].ptr, len);
		str[len] = '\0';
		return str;
	}
	*str = '\0';
	return str;
}

char *x_recatch_replace(x_recatch *catch, char *new_str, const char *rpl_str, int id)
{
	char *ret_str = new_str;
	const char *last = catch->items[0].ptr;
	for(int index = 1, rpLen = strlen(rpl_str); index < catch->count; index++)
		if (id == catch->items[index].id) {
			if (last > catch->items[index].ptr)
				last = catch->items[index].ptr;
			strncpy(new_str, last, catch->items[index].ptr - last);
			new_str += catch->items[index].ptr - last;
			strcpy(new_str, rpl_str);
			new_str += rpLen;
			last = catch->items[index].ptr + catch->items[index].len;
		}
	int len = catch->items[0].ptr + catch->items[0].len - last;
	strncpy(new_str, last, len);
	new_str[len] = '\0';
	return ret_str;
}

char *x_recatch_put(x_recatch *catch, char *new_str, const char *put_str)
{
	char *ret_str = new_str;
	while (*put_str)
		switch (*put_str) {
			case '#':
				if (*++put_str == '#')
					*new_str++ = *put_str++;
				else {
					int index = atoi(put_str);
					x_recatch_fetch(catch, new_str, index);
					new_str += x_recatch_len_at(catch, index);
					put_str += digit_count(put_str);
				}
				break;
			default:
				*new_str++ = *put_str++;
		}
	*new_str = '\0';
	return ret_str;
}

