#include "x/pathset.h"
#include "x/strbuf.h"
#include "x/test.h"
#include "x/printf.h"
#include <unistd.h>
#include <stdio.h>
#include <time.h>

#define ASSERT_EQUAL(ex, ac) { \
	x_strbuf strbuf; \
	x_pathset_dump(ac, &strbuf); \
	ut_assert_str_equal(r, ex, x_strbuf_str(&strbuf)); \
}

static void insert(ut_runner *r)
{
	x_pathset pathset;
	x_pathset_init(&pathset);

	{
		x_pathset_insert(&pathset, 0x01, "/a/b/c", false);
		x_pathset_insert(&pathset, 0x02, "/a/b/d", false);
		x_pathset_insert(&pathset, 0x04, "/a/b", false);
		x_uchar ex[] = x_u(
		     "0x05 /a/b/c, "
		     "0x06 /a/b/d, "
		     "0x04 /a/b, ");
		ASSERT_EQUAL(ex, &pathset);
	}

	{
		x_pathset_clear(&pathset);
		ASSERT_EQUAL("", &pathset);
	}

	{
		x_pathset_clear(&pathset);
		x_pathset_insert(&pathset, 0x05, "/a/b/c", false);
		x_pathset_insert(&pathset, 0x06, "/a/b/d", false);
		x_pathset_insert(&pathset, 0x04, "/a/b/e", false);
		x_pathset_remove(&pathset, 0x04, "/a/b", false);
		x_uchar ex[] = x_u(\
		     "0x01 /a/b/c, "
		     "0x02 /a/b/d, ");
		ASSERT_EQUAL(ex, &pathset);
	}

	{
		x_pathset_clear(&pathset);
		x_pathset_insert(&pathset, 0x01, "/", false);
		x_uchar ex[] = "0x01 /, ";
		ASSERT_EQUAL(ex, &pathset);
	}

	{
		x_pathset_clear(&pathset);
		x_pathset_insert(&pathset, 0x01, "/", false);
		x_pathset_insert(&pathset, 0x02, "/a", false);
		x_pathset_insert(&pathset, 0x04, "/a/b", false);
		x_uchar ex[] = \
		     "0x01 /, "
		     "0x03 /a, "
		     "0x07 /a/b, ";
		ASSERT_EQUAL(ex, &pathset);
	}

	{
		x_pathset_clear(&pathset);
		x_pathset_insert(&pathset, 0x01, "/a/b/c", false);
		x_pathset_insert(&pathset, 0x03, "/a/b", false);
		x_uchar ex[] =  "0x03 /a/b, ";
		ASSERT_EQUAL(ex, &pathset);
	}

	{
		x_pathset_clear(&pathset);
		x_pathset_insert(&pathset, 0x03, "/a/d", false);
		x_pathset_insert(&pathset, 0x01, "/a/b/c", false);
		x_pathset_remove(&pathset, 0x03, "/a/b", false);
		x_uchar ex[] =  x_u("0x03 /a/d, ");
		ASSERT_EQUAL(ex, &pathset);
	}

	{
		x_pathset_clear(&pathset);
		x_pathset_insert(&pathset, 0x03, "/", false);
		x_pathset_remove(&pathset, 0x01, "/", false);
		x_uchar ex[] =  "0x02 /, ";
		ASSERT_EQUAL(ex, &pathset);
	}

	{
		x_pathset_clear(&pathset);
		x_pathset_insert(&pathset, 0x01, "/a/b", false);
		x_pathset_remove(&pathset, 0x01, "/a/b/c", false);
		x_uchar ex[] = \
		     "0x01 /a/b, "
		     "0x00 /a/b/c, ";
		ASSERT_EQUAL(ex, &pathset);
	}

	{
		x_pathset_clear(&pathset);
		x_pathset_insert(&pathset, 0x01, "/a/b", false);
		x_pathset_insert(&pathset, 0x01, "/a/c/d", false);
		x_uchar ex[] = \
		     "0x01 /a/b, "
		     "0x01 /a/c/d, ";
		ASSERT_EQUAL(ex, &pathset);
	}

	{
		x_pathset_clear(&pathset);
		x_pathset_insert(&pathset, 0x03, "/a/b", false);
		x_pathset_insert(&pathset, 0x03, "/a/c", true);
		x_uchar ex[] = \
		     "0x03 /a/b, "
		     "0x03 /a/c, ";
		ASSERT_EQUAL(ex, &pathset);
		ut_assert_int_equal(r, 0x01, (x_pathset_mask(&pathset, "/a/c") & 0x01));
	}
}

void find_top(ut_runner *r)
{
	x_pathset pathset;
	x_pathset_init(&pathset);

	x_pathset_insert(&pathset, 0x01, "/a", false);
	x_pathset_insert(&pathset, 0x02, "/a/b", false);
	x_pathset_insert(&pathset, 0x02, "/a/c", false);
	x_pathset_insert(&pathset, 0x01, "/b/a", false);
	x_pathset_insert(&pathset, 0x01, "/b/b", false);

	x_list l;
	x_pathset_find_top(&pathset, &l);
	int i = 0;
	x_uchar *expect_list[] = { x_u("/a"), x_u("/b/a"), x_u("/b/b") };
	x_list_foreach(cur, &l) {
		x_pathset_entry *ent = x_container_of(cur, x_pathset_entry, link);
		ut_assert_int_equal(r, 0, x_ustrcmp(ent->path, expect_list[i++]));
	}
	x_pathset_free_entry_list(&l);

}

void pathset_test_init(ut_suite *s)
{
	ut_suite_init(s, __FILE__);
	ut_suite_add(s, insert);
	ut_suite_add(s, find_top);
}

