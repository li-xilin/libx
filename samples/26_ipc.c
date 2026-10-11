#include "x/ipcsock.h"
#include "x/path.h"
#include "x/printf.h"
#include "x/uchar.h"
#include "x/errno.h"
#include <stdio.h>

static int send_and_recv(x_sock fd, const char *message)
{
	char buf[128];
	strcpy(buf, message);
	// sprintf(buf, );
	int len = send(fd, buf, strlen(buf), MSG_NOSIGNAL);
	if (len < 0) {
		fprintf(stderr, "failed to send (%d)\n", errno);
		return -1;
	}
	len = recv(fd, buf, sizeof buf - 1, MSG_NOSIGNAL);
	if (len < 0) {
		fprintf(stderr, "failed to recv (%d)\n", errno);
		return -1;
	}
	if (len == 0)
		return 0;
	buf[len] = '\0';
	printf("%s\n", buf);
	return 0;
}

int main(int argc, char *argv[])
{
	if (argc < 2) {
		fprintf(stderr, "expect '-c' for client and '-s' for server\n");
		return 1;
	}

	x_uchar path[X_PATH_MAX];
	if (!x_path_tmpdir(path, sizeof path)) {
		fprintf(stderr, "failed to get temp directory\n");
		return 1;
	}
	x_ustrcat(path, X_PATH_SEP x_u("test_ipc_file"));
	x_sock_init();

	if (strcmp(argv[1], "-c") == 0) {
		x_printf(x_u("connect to %S\n"), path);
		x_sock fd = x_ipcsock_connect(path, false);
		if (fd == X_BADSOCK) {
			fprintf(stderr, "failed to connect unix socket file (%d)\n", errno);
			return 1;
		}
		x_ipcsock_set_fastpath(fd, true);
		if (send_and_recv(fd, "Hello, I am client!"))
			return 1;
		
	}
	else if (strcmp(argv[1], "-s") == 0) {
		x_printf(x_u("listen at %S\n"), path);
		x_sock listen_fd = x_ipcsock_listen(path, false);
		if (listen_fd == X_BADSOCK) {
			fprintf(stderr, "failed to listen on unix socket file (%d)\n", errno);
			return 1;
		}
		x_sock fd;
		while ((fd = accept(listen_fd, NULL, NULL))) {
			x_ipcsock_set_fastpath(fd, true);
			if (send_and_recv(fd, "Hello, I am server!"))
				return 1;
			x_sock_close(fd);
		}
	}
	else {
		fprintf(stderr, "unknown option %s\n", argv[1]);
	}

	x_sock_exit();
}
