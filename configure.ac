AC_INIT([libx], [1.1.4], [lixilin@gmx.com])
AC_CONFIG_AUX_DIR([build-aux])
AC_CANONICAL_HOST
AC_CONFIG_MACRO_DIR([m4])
AC_CONFIG_HEADERS([config.h])

AC_PROG_CC
AC_PROG_CC_C99
AC_EXEEXT
AC_ENABLE_SHARED

AM_PROG_CC_C_O
AM_PROG_AR
AM_INIT_AUTOMAKE([foreign tar-pax no-dist-gzip dist-bzip2 1.6 subdir-objects])
m4_ifdef([AM_SILENT_RULES], [AM_SILENT_RULES([yes])])
LT_INIT

AC_DEFUN([AC_CHECK_HEADERS_ERROR], [
	AC_CHECK_HEADERS(m4_normalize([$1]), [], [AC_MSG_ERROR([required header not found. ])], [$2])
])

AC_DEFUN([AC_CHECK_POSIX_HEADERS], [
	AC_CHECK_HEADERS_ERROR([
		pthread.h
		dirent.h
		dlfcn.h
		unistd.h
		utime.h
		termios.h
		fcntl.h
		arpa/inet.h
		sys/ioctl.h
		sys/socket.h
		sys/stat.h
		sys/time.h
		sys/types.h
		sys/wait.h
		netinet/in.h
		netinet/tcp.h
	])
])

AC_DEFUN([AC_CHECK_WIN32_HEADERS], [
	AC_CHECK_HEADERS([afunix.h])
	AC_CHECK_HEADERS_ERROR([
		windef.h
		winbase.h
		winver.h
	], [
		#include <windef.h>
	])
	AC_CHECK_HEADERS_ERROR([
		io.h
		windows.h
		wincrypt.h
		winerror.h
		winioctl.h
		winuser.h
		libloaderapi.h
		stringapiset.h
		strsafe.h
		synchapi.h
		errhandlingapi.h
		shellapi.h
		tchar.h
		shlwapi.h
		fileapi.h
		process.h
		fcntl.h
		sysinfoapi.h
		tlhelp32.h
		dbghelp.h
	], [
		#include <windef.h>
		#include <winbase.h>
		#include <winver.h>
	])
])

AC_DEFUN([AC_CHECK_STDC_HEADERS], [
	AC_CHECK_HEADERS_ERROR([
		setjmp.h
		assert.h
		ctype.h
		errno.h
		float.h
		inttypes.h
		limits.h
		locale.h
		math.h
		memory.h
		signal.h
		stdarg.h
		stdbool.h
		stddef.h
		stdint.h
		stdio.h
		stdlib.h
		string.h
		time.h
		wchar.h
	])
])

AC_DEFUN([AC_CHECK_LINUX_HEADERS], [
	AC_CHECK_HEADERS_ERROR([
		sys/syscall.h
	])
])

AC_CHECK_STDC_HEADERS

AC_CHECK_HEADERS(m4_normalize([
	endian.h
	sys/endian.h
]))

case "$host" in
*-*-linux* | *-*-uclinux*)
	PLATFORM=linux
	LT_INIT
	AC_CHECK_LINUX_HEADERS
	;;
*-*-mingw* | *-*-cygwin*)
	PLATFORM=win32
	AC_CHECK_TOOL([DLLTOOL], [dlltool], [])
	lt_cv_sys_path_separator=':'
	lt_cv_to_host_file_cmd=func_convert_file_noop
	lt_cv_to_tool_file_cmd=func_convert_file_noop
	LT_INIT([win32-dll pic-only])
	AC_CHECK_WIN32_HEADERS
	;;
*-*-freebsd* | *-*-kfreebsd*-gnu)
	PLATFORM=freebsd
	LT_INIT
	LDFLAGS="$LDFLAGS -L/usr/local/lib"
	CFLAGS="$CFLAGS -I/usr/local/include"
	AC_CHECK_POSIX_HEADERS
	AC_CHECK_LINUX_HEADERS
	;;
*-*-darwin*)
	PLATFORM=darwin
	LT_INIT([dlopen])
	AC_CHECK_POSIX_HEADERS
	;;
*-*-openbsd*)
	PLATFORM=openbsd
	LT_INIT
	AC_CHECK_POSIX_HEADERS
	;;
*-*-netbsd*)
	PLATFORM=netbsd
	LT_INIT
	AC_CHECK_POSIX_HEADERS
	;;
*-*-solaris* | *-*-sunos*)
	PLATFORM=solaris
	LT_INIT
	LIBS="$LIBS -lsocket -lnsl"
	AC_CHECK_POSIX_HEADERS
	;;
*-*-aix*)
	PLATFORM=aix
	LT_INIT
	AC_CHECK_POSIX_HEADERS
	;;
*-*-hpux*)
	PLATFORM=hpux
	LT_INIT
	AC_CHECK_POSIX_HEADERS
	;;
*-*-gnu* | *-*-*bsd* | *-*-sysv*)
	PLATFORM=unix
	LT_INIT
	AC_CHECK_POSIX_HEADERS
	;;
*)
	AC_MSG_WARN([unsupported platform: $host, trying generic configuration])
	PLATFORM=generic
	LT_INIT
	AC_CHECK_POSIX_HEADERS
	;;
esac

AC_ARG_ENABLE([network], [AS_HELP_STRING([--disable-network], [Disable network functions])],
			  [disable_network=yes],
			  [disable_network=no])
AM_CONDITIONAL([ENABLE_NETWORK], [test "x$disable_network" = "xno"])
if test "x$disable_network" = "xno"; then
	if test "x$PLATFORM" = xwin32; then
		AC_CHECK_HEADERS_ERROR([
			winsock2.h
			ws2tcpip.h
		])
		AM_CONDITIONAL([HAVE_SYS_EPOLL_H], [true])
		AM_CONDITIONAL([HAVE_POLL_H], [false])
	else
		AC_CHECK_HEADERS([sys/epoll.h kqueue.h poll.h])
		if test "x$ac_cv_header_sys_epoll_h" = "xno" -a "x$ac_cv_header_poll_h" = "xno"; then
			AC_MSG_ERROR([polling header not found ])
		fi
		AM_CONDITIONAL([HAVE_SYS_EPOLL_H], [test "x$ac_cv_header_sys_epoll_h" = "xyes"])
		AM_CONDITIONAL([HAVE_POLL_H], [test "x$ac_cv_header_poll_h" = "xyes"])
	fi
fi

AC_ARG_ENABLE([edit], [AS_HELP_STRING([--disable-edit], [Disable readline/edit functions])],
			  [disable_edit=yes],
			  [disable_edit=no])
AM_CONDITIONAL([ENABLE_EDIT], [test "x$disable_edit" = "xno"])

AC_ARG_ENABLE([json], [AS_HELP_STRING([--disable-json], [Disable json and json-path functions])],
			  [disable_json=yes],
			  [disable_json=no])
AM_CONDITIONAL([ENABLE_JSON], [test "x$disable_json" = "xno"])

AC_ARG_ENABLE([regex], [AS_HELP_STRING([--disable-regex], [Disable regex functions])],
			  [disable_regex=yes],
			  [disable_regex=no])
AM_CONDITIONAL([ENABLE_REGEX], [test "x$disable_regex" = "xno"])

AC_ARG_ENABLE([crypto], [AS_HELP_STRING([--disable-crypto], [Disable crypto functions])],
			  [disable_crypto=yes],
			  [disable_crypto=no])
AM_CONDITIONAL([ENABLE_CRYPTO], [test "x$disable_crypto" = "xno"])

AC_ARG_ENABLE([charmap], [AS_HELP_STRING([--disable-charmap], [Disable charactor encoding convertion])],
			  [disable_charmap=yes],
			  [disable_charmap=no])
AM_CONDITIONAL([ENABLE_CHARMAP], [test "x$disable_charmap" = "xno"])


regular_CFLAGS="-Wall -Werror -std=c99 -Wshadow -Wstrict-prototypes -Wformat=2 -pipe"

AM_CONDITIONAL([WINDOWS], [test "x$host_os" = xmingw32 || test "x$host_os" = xcygwin || test "x$host_os" = xmsys])
AM_COND_IF([WINDOWS],
	regular_CFLAGS="$regular_CFLAGS -D_WIN32_WINNT=0x0600 -DWIN32_LEAN_AND_MEAN",
	regular_CFLAGS="$regular_CFLAGS -D_POSIX_C_SOURCE=200112L -D_XOPEN_SOURCE=500")

AC_SUBST([regular_CFLAGS])
AC_SUBST([LT_VERSION_INFO], ["1:0:0"])
AC_SUBST([VERSION_MAJOR], [`echo "$PACKAGE_VERSION" | cut -d. -f1`])
AC_SUBST([VERSION_MINOR], [`echo "$PACKAGE_VERSION" | cut -d. -f2`])
AC_SUBST([VERSION_PATCH], [`echo "$PACKAGE_VERSION" | cut -d. -f3`])
AC_CONFIG_FILES([Makefile include/Makefile libx/Makefile libx.pc test/Makefile samples/Makefile])
AC_OUTPUT

