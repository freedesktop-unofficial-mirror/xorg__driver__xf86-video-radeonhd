# Process this file with autoconf to produce a configure script

AC_PREREQ(2.57)
AC_INIT([xf86-video-radeonhd],
        0.0.2,
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

AH_TOP([#ifndef _CONFIG_H])
AH_TOP([# define _CONFIG_H])
AH_TOP([# include "xorg-server.h"])
AH_BOTTOM([#endif])

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
PKG_CHECK_MODULES(XORG, [xorg-server xproto fontsproto $REQUIRED_MODULES])
sdkdir=$(pkg-config --variable=sdkdir xorg-server)

# Header files checks.
AC_HEADER_STDC

SAVED_CFLAGS="$CFLAGS"
CFLAGS="$CFLAGS $XORG_CFLAGS"

# Check whether MonRec has a reducedblanking member.
AC_CHECK_MEMBER([MonRec.reducedblanking],
         [MONREC_HAS_REDUCED=yes], [MONREC_HAS_REDUCED=no],
         [#include "${sdkdir}/xf86.h"])
if test "x$MONREC_HAS_REDUCED" = xyes; then
        AC_DEFINE(MONREC_HAS_REDUCED, 1, [MonRec has member reducedblanking])
fi

# Check whether MonRec has a maxPixClock member.
AC_CHECK_MEMBER([MonRec.maxPixClock],
         [MONREC_HAS_BANDWIDTH=yes], [MONREC_HAS_BANDWIDTH=no],
         [#include "${sdkdir}/xf86.h"])
if test "x$MONREC_HAS_BANDWIDTH" = xyes; then
        AC_DEFINE(MONREC_HAS_BANDWIDTH, 1, [MonRec has member maxPixClock])
fi

CFLAGS="$SAVED_CFLAGS"

AC_ARG_ENABLE(atombios, AC_HELP_STRING([--disable-atombios],
                                  [Disable AtomBIOS support [[default=no]]]),
              [do_atombios="$enableval"],
              [do_atombios=yes])

AC_ARG_ENABLE(atombios-parser, AC_HELP_STRING([--disable-atombios-parser],
                                  [Disable AtomBIOS parser support [[default=no]]]),
              [do_atombios_parser="$enableval"],
              [do_atombios_parser=yes])

AC_CHECK_FILE([${srcdir}/src/AtomBios/includes/atombios.h],
              [have_atombios_h="yes"], [have_atombios_h="no"])

AC_CHECK_FILE([${srcdir}/src/AtomBios/includes/Decoder.h],
              [have_atombios_parser="yes"], [have_atombios_parser="no"])

AC_MSG_CHECKING([whether to include AtomBIOS])
if test "$have_atombios_h" = yes -a \
	"$do_atombios" = yes; then
	support_atombios="yes"
        AC_DEFINE(ATOM_BIOS, 1, [Add support for AtomBIOS tables])
else
	support_atombios="no"
fi

AC_MSG_RESULT([$support_atombios])

AC_MSG_CHECKING([whether to include AtomBIOS parser])
if test "$support_atombios" = yes -a \
	"$do_atombios_parser" = yes -a \
	"$have_atombios_parser" = yes; then
	support_atombios_parser="yes"
        AC_DEFINE(ATOM_BIOS_PARSER, 1, [Add support for AtomBIOS code])
else
	support_atombios_parser="no"
fi

AC_MSG_RESULT([$support_atombios_parser])

AM_CONDITIONAL(ATOM_BIOS, test x$support_atombios = xyes)

AM_CONDITIONAL(ATOM_BIOS_PARSER, test x$support_atombios_parser = xyes)

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
