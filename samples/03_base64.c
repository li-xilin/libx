#include "x/base64.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
int main(void)
{
	size_t len;
	char text[] = "Hello world";
	char *enc = x_base64_encode(text, sizeof text);
	char *dec = x_base64_decode(enc, strlen(enc), &len);

	printf("origin: %s\n", text);
	printf("encode: %s\n", enc);
	printf("decode: %s\n", dec);
	printf("decoded_size: %zu\n", len);

	free(enc);
	free(dec);
}
