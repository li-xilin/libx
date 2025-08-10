#include "x/json.h"
#include "x/memory.h"
#include <stdio.h>
#include <string.h>

int main(void)
{
	x_json *tree = NULL;
	const char test_json[] = "{\"Image\":{" 
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

	char *out = x_json_print(tree, true);
	printf("%s\n", out);

	printf("%s = %lf\n", tree->value.child->value.child->string, x_json_number(tree->value.child->value.child));
	printf("%s = %lf\n", tree->value.child->value.child->next->string, x_json_number(tree->value.child->value.child->next));

	x_json_free(tree);
	x_free(out);
}
