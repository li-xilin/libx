AC_INIT([libx], [1.1.4], [lixilin@gmx.com])
AC_CONFIG_AUX_DIR([build-aux])
AC_CANONICAL_HOST
AC_CONFIG_MACRO_DIR([m4])
AC_CONFIG_HEADERS([config.h])

AC_PROG_CC([gcc clang cc])
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


case "$host" in
*-*-linux-android* | *-*-android*)
    PLATFORM=android
    LT_INIT
    ;;
*-*-linux* | *-*-uclinux*)
	PLATFORM=linux
	regular_CFLAGS="$regular_CFLAGS -D_XOPEN_SOURCE=600"
	LT_INIT
	;;
*-*-mingw* | *-*-cygwin*)
	PLATFORM=win32
	AC_CHECK_TOOL([DLLTOOL], [dlltool], [])
	lt_cv_sys_path_separator=':'
	lt_cv_to_host_file_cmd=func_convert_file_noop
	lt_cv_to_tool_file_cmd=func_convert_file_noop
	regular_CFLAGS="$regular_CFLAG -static-libgcc -D_WIN32_WINNT=0x0600 -DWIN32_LEAN_AND_MEAN"
	LT_INIT([win32-dll pic-only])
	;;
*-*-freebsd* | *-*-kfreebsd*-gnu)
	PLATFORM=freebsd
	LT_INIT
	LDFLAGS="-lexecinfo $LDFLAGS"
	regular_CFLAGS="$regular_CFLAGS -D_XOPEN_SOURCE=600 -D__BSD_VISIBLE"
	;;
*-*-darwin*)
	PLATFORM=darwin
	LT_INIT([dlopen])
	regular_CFLAGS="$regular_CFLAGS -D_XOPEN_SOURCE=600 -D_DARWIN_C_SOURCE "
	;;
*-*-openbsd*)
	PLATFORM=openbsd
	LT_INIT
	regular_CFLAGS="$regular_CFLAGS -D_XOPEN_SOURCE=600 -D__BSD_VISIBLE"
	;;
*-*-netbsd*)
	PLATFORM=netbsd
	LT_INIT
	regular_CFLAGS="$regular_CFLAGS -D_XOPEN_SOURCE=600 -D_NETBSD_SOURCE"
	;;
*-*-aix*)
	PLATFORM=aix
	LT_INIT
	regular_CFLAGS="$regular_CFLAGS -D_XOPEN_SOURCE=600 -D_BSD"
	;;
*-*-gnu* | *-*-*bsd* | *-*-sysv*)
	PLATFORM=unix
	LT_INIT
	regular_CFLAGS="$regular_CFLAGS -D_XOPEN_SOURCE=600"
	;;
*)
	AC_MSG_WARN([unsupported platform: $host, trying generic configuration])
	PLATFORM=generic
	LT_INIT
	;;
esac

AC_MSG_CHECKING([whether the linker supports version scripts])
save_LIBS="$LIBS"
cat > conftest.map <<EOF
VERS_1.0 {
    global:
        some_function;
    local:
        *;
};
EOF
LIBS="$save_LIBS -Wl,--version-script=conftest.map"
AC_LINK_IFELSE(
    [AC_LANG_PROGRAM([], [])], # 一个空的测试程序
    [ac_cv_have_ld_version_script=yes],
    [ac_cv_have_ld_version_script=no]
)
LIBS="$save_LIBS"
rm -f conftest.map
AC_MSG_RESULT([$ac_cv_have_ld_version_script])
AM_CONDITIONAL([HAVE_LD_VERSION_SCRIPT], [test "x$ac_cv_have_ld_version_script" = "xyes"])

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
else
	AM_CONDITIONAL([HAVE_SYS_EPOLL_H], [false])
	AM_CONDITIONAL([HAVE_POLL_H], [false])
fi

AC_ARG_ENABLE([debug], [AS_HELP_STRING([--enable-debug], [enable debug build (default: no)])],
			  [enable_debug=$enableval],
			  [enable_debug=no])
regular_CFLAGS="$regular_CFLAGS -std=c99 -Wstrict-prototypes -Wformat=2 -pipe"

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
		sys/param.h
	], [
		#include <windef.h>
		#include <winbase.h>
		#include <winver.h>
	])
	AC_CHECK_HEADERS([afunix.h], [], [], [#include <winsock2.h>])
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
	AC_CHECK_HEADERS([
		linux/stat.h
		linux/limits.h
	])
])

AC_DEFUN([AC_CHECK_UNIX_HEADERS], [
	AC_CHECK_HEADERS([
		execinfo.h
	])
])

save_CFLAGS="$CFLAGS"
save_LIBS="$LIBS"
CFLAGS="$regular_CFLAGS $CFLAGS"
AC_CHECK_STDC_HEADERS
if test "x$PLATFORM" = "xwin32"; then
	AC_CHECK_WIN32_HEADERS
else
	if test "x$PLATFORM" = "xlinux"; then
		AC_CHECK_LINUX_HEADERS
	fi
	AC_CHECK_POSIX_HEADERS
	AC_CHECK_UNIX_HEADERS
fi
AC_MSG_CHECKING([for _Unwind_Backtrace])
AC_LINK_IFELSE([AC_LANG_PROGRAM([[
	#include <unwind.h>
]], [[
	extern int _Unwind_Backtrace(void *, void *);
	_Unwind_Backtrace(NULL, NULL);
]])], [
	AC_MSG_RESULT([yes])
	AC_DEFINE([HAVE__UNWIND_BACKTRACE], [1], ["Define if _Unwind_Backtrace is available"])
], [
	AC_MSG_RESULT([no])
])
AC_MSG_RESULT([yes])
CFLAGS="$save_CFLAGS"
LIBS="$save_LIBS"

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

AH_TEMPLATE([X_NDEBUG],
	[Define to 1 when building without debug support, so unsafe/debug-only code is compiled out.])

AS_IF([test "x$enable_debug" = "xyes"], [
	PC_DEBUG_CFLAGS="-DX_DEBUG=1"
	CFLAGS="$CFLAGS -Wpedantic -g3 -O0 -Wall -Werror $PC_DEBUG_CFLAGS"
], [
	CFLAGS="$CFLAGS -O2 -DNDEBUG"
])
AC_SUBST([PC_DEBUG_CFLAGS])

AM_CONDITIONAL([WINDOWS], [test "x$PLATFORM" = xwin32])
AM_CONDITIONAL([ANDROID], [test "x$PLATFORM" = xandroid])

AC_SUBST([regular_CFLAGS])
AC_SUBST([LT_VERSION_INFO], ["1:0:0"])
AC_SUBST([VERSION_MAJOR], [`echo "$PACKAGE_VERSION" | cut -d. -f1`])
AC_SUBST([VERSION_MINOR], [`echo "$PACKAGE_VERSION" | cut -d. -f2`])
AC_SUBST([VERSION_PATCH], [`echo "$PACKAGE_VERSION" | cut -d. -f3`])
AC_CONFIG_FILES([Makefile include/Makefile libx/Makefile libx.pc test/Makefile samples/Makefile])
AC_OUTPUT

