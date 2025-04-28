#!/bin/sh
set -e
check_tool() {
    if ! command -v "$1" >/dev/null 2>&1; then
        echo "Error: '$1' is required but not installed." >&2
        exit 1
    fi
}

check_tool aclocal
check_tool autoconf
check_tool automake
check_tool libtoolize || check_tool glibtoolize

rm -rf autom4te.cache
rm -f aclocal.m4 configure config.h.in

mkdir -p build-aux m4

echo "Running aclocal..."
aclocal -I m4

echo "Running autoheader..."
autoheader

echo "Running autoconf..."
autoconf -i

echo "Running libtoolize..."
if [ "$(uname)" = "Darwin" ]; then
	glibtoolize --copy --force
else
	libtoolize --copy --force
fi

echo "Running automake..."
automake --add-missing --copy --foreign

libtoolize
