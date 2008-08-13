#! /bin/sh

srcdir=`dirname "$0"`
test -z "$srcdir" && srcdir=.

ORIGDIR=`pwd`
cd "$srcdir";

rm -rf autom4te.cache
autoreconf -v --install || exit 1
cd "$ORIGDIR" || exit $?

"$srcdir"/configure --enable-maintainer-mode "$@"
