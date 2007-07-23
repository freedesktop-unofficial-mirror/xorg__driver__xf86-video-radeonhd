# Process this file with autoconf to produce a configure script

AC_PREREQ(2.57)
AC_INIT([xf86-video-radeonhd],
        1.1.1,
        [https://bugs.freedesktop.org/enter_bug.cgi?product=xorg],
        xf86-video-radeonhd)

AC_CONFIG_SRCDIR([Makefile.am])
AM_CONFIG_HEADER([config.h])
AC_CONFIG_AUX_DIR(.)

AM_INIT_AUTOMAKE([dist-bzip2])

AM_MAINTAINER_MODE

# Program checks.
AC_DISABLE_STATIC
AC_PROG_LIBTOOL
AC_PROG_CC

if test "x$GCC" = "xyes"; then
        CFLAGS="$CFLAGS -Wall -pedantic"
fi

AH_TOP([#include "xorg-server.h"])

AC_ARG_WITH(xorg-module-dir,
            AC_HELP_STRING([--with-xorg-module-dir=DIR],
                           [Default xorg module directory [[default=$libdir/xorg/modules]]]),
            [moduledir="$withval"],
            [moduledir="$libdir/xorg/modules"])

# Extensions checks.
XORG_DRIVER_CHECK_EXT(RANDR, randrproto)
XORG_DRIVER_CHECK_EXT(RENDER, renderproto)
XORG_DRIVER_CHECK_EXT(XV, videoproto)
XORG_DRIVER_CHECK_EXT(DPMSExtension, xextproto)
# @@@@ We don't do DGA any more

# pkg-config packages checks
# @@@@ need to check for the right server version once RandR 1.2 is in.
# @@@ do we really need fontsproto here?
PKG_CHECK_MODULES(XORG, [xorg-server >= 1.0.99.901 xproto fontsproto $REQUIRED_MODULES])
sdkdir=$(pkg-config --variable=sdkdir xorg-server)

# Header files checks.
AC_HEADER_STDC

# libraries checks.

AC_SUBST([XORG_CFLAGS])
AC_SUBST([moduledir])

DRIVER_NAME=radeonhd
AC_SUBST([DRIVER_NAME])

# XORG_MANPAGE_SECTIONS
XORG_RELEASE_VERSION

AC_OUTPUT([
	Makefile
	src/Makefile
])
