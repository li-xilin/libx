#include "x/json.h"
#include "x/jpath.h"
#include "x/memory.h"
#include <stdio.h>
#include <string.h>

int main(void)
{
	x_json *tree = NULL;
	const char test_json[] =
		"{"
			"\"Image\":{" 
				"\"Width\":800," 
				"\"Height\":600," 
				"\"Title\":\"Viewfrom15thFloor\"," 
				"\"Thumbnail\":{" 
					"\"Url\":\"http:/*www.example.com/image/481989943\"," 
					"\"Height\":125," 
					"\"Width\":\"100\"" 
				"}," 
				"\"IDs\":[116,943,234,38793]" 
			"}" 
		"}";
    tree = x_json_parse(test_json);

	{
		printf("# Print content in formated\n");
		char *out = x_json_print(tree, true);
		printf("%s\n", out);
		x_free(out);
	}

	{
		printf("# Print content in non-formated\n");
		char *out = x_json_print(tree, false);
		printf("%s\n", out);
		x_free(out);
	}

	{
		printf("# Access node with tree structure\n");
		printf("%s = %lf\n",
				tree->value.child->value.child->string,
				x_json_number(tree->value.child->value.child));
		printf("%s = %lf\n",
				tree->value.child->value.child->next->string,
				x_json_number(tree->value.child->value.child->next));
	}

	{
		
		printf("# Search nodes with json path\n");
		size_t pos, i = 0;
		x_jpath *path = x_jpath_parse("$.Image.IDs[*]", &pos);;
		x_jpath_result *r = x_jpath_result_create();
		x_jpath_match(tree, path, true, r);
		x_jpath_result_foreach(e, r)
			printf("IDs[%d] = %d\n", (int)i++, (int)x_json_number(e));
	}

	x_json_free(tree);
}
