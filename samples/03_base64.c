#include "x/base64.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
int main()
{
	char text[] = "Hello world";
	char *enc = x_base64_encode(text, sizeof text);
	char *dec = x_base64_decode(enc, strlen(enc));

	printf("origin: %s\n", text);
	printf("encode: %s\n", enc);
	printf("decode: %s\n", dec);

	free(enc);
	free(dec);
}
