#include "x/asynet.h"
#include "x/reactor.h"
#include <stdio.h>
#include <stdlib.h>

static x_reactor r;

static void on_dns_result(int err, const char *name, x_ipaddr *addrs, size_t n, void *user)
{
	char buf[256];
	if (err) {
		fprintf(stderr, "error: %s\n", gai_strerror(err));
		return;
	}
	printf("result of %s:\n", name);
	for (int i = 0; i < n; i++)
		printf("  [%02d] %s\n", i, inet_ntop(addrs[i].family,  &addrs[i].un, buf, sizeof buf));
}

int main(int argc, char *argv[])
{
	if (argc < 2) {
		fprintf(stderr, "expect doamin name.\n");
		exit(1);
	}
	if (x_reactor_init(&r)) {
		perror("x_reactor_init");
		exit(1);
	}
	x_resolver *resolver = x_resolver_create(&r);
	
	for (int i = 1; i < argc; i++) {
		x_resolver_query(resolver, argv[i], on_dns_result, NULL);
		fprintf(stderr, "sent query of '%s'\n", argv[i]);
	}

	while (x_reactor_wait(&r) > 0)
		x_reactor_do_events(&r);

	x_reactor_free(&r);
}

