AC_INIT([libx], [1.0.0])
AC_CONFIG_AUX_DIR([build-aux])
AC_CANONICAL_HOST
AC_CONFIG_MACRO_DIR([m4])
AC_CONFIG_HEADERS([config.h])
AM_INIT_AUTOMAKE([foreign tar-pax no-dist-gzip dist-bzip2 1.6 subdir-objects])

m4_ifdef([AM_SILENT_RULES], [AM_SILENT_RULES([yes])])

AC_PROG_CC
AM_PROG_CC_C_O
AC_EXEEXT
AC_ENABLE_STATIC
AC_ENABLE_SHARED
AC_LIBTOOL_WIN32_DLL
AC_CHECK_TOOL([DLLTOOL], [dlltool], [])
lt_cv_sys_path_separator=':'
lt_cv_to_host_file_cmd=func_convert_file_noop
lt_cv_to_tool_file_cmd=func_convert_file_noop
LT_INIT([win32-dll, shared])

regular_CFLAGS="-Wall -Werror --pedantic -std=c99 -Wshadow -Wstrict-prototypes -Wformat=2 -pipe"
AM_CONDITIONAL([WINDOWS], [test "x$host_os" = xmingw32 || test "x$host_os" = xcygwin || test "x$host_os" = xmsys])
AM_COND_IF([WINDOWS],
	regular_CFLAGS="$regular_CFLAGS -D_WIN32_WINNT=0x0600 -DWIN32_LEAN_AND_MEAN -D__USE_MINGW_ANSI_STDIO -mwindows",
	regular_CFLAGS="$regular_CFLAGS -D_POSIX_C_SOURCE=200112L")

AC_SUBST([regular_CFLAGS])
AC_SUBST([LT_VERSION_INFO], ["1:0:0"])
AC_CONFIG_FILES([Makefile libx/Makefile libx.pc include/Makefile samples/Makefile])
AC_OUTPUT
