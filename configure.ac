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

# compiler flags

WARN_CFLAGS=""

# Extra Warnings
# Due to xorg/os.h: -Wno-variadic-macros
AC_LANG_CONFTEST([AC_LANG_PROGRAM([[]])])
for w in \
  -Wall \
  -Wextra -Wno-unused-parameter \
  -pedantic -Wno-long-long -Wno-variadic-macros \
  -Werror-implicit-function-declaration \
  -Wstrict-aliasing -Wstrict-overflow \
  -Wpointer-arith -Woverlength-strings -Wvolatile-register-var -Wshadow \
  -Winit-self -Wswitch-enum -Wbad-function-cast \
  -Wstrict-prototypes -Wold-style-definition \
  -Wnested-externs -Wunreachable-code -Wdisabled-optimization \
  -Wi-dont-exist-go-away-for-testing ;
do
  AC_MSG_CHECKING([whether $CC supports $w])
  if $CC $w -c -o /dev/null conftest.c >/dev/null 2>&1 ; then
    AC_MSG_RESULT([yes])
    WARN_CFLAGS="$WARN_CFLAGS $w"
  else
    AC_MSG_RESULT([no])
  fi
done
# Optimizations
case "x$CFLAGS" in
dnl Automake needs extra escaping
  [*-O[2-9]*)]
    AC_MSG_NOTICE([Doing optimized build, checking for additional options])
    for w in -finline-functions ; do
      AC_MSG_CHECKING([whether $CC supports $w])
      if $CC $w -c -o /dev/null conftest.c >/dev/null 2>&1 ; then
	AC_MSG_RESULT([yes])
        CFLAGS="$CFLAGS -finline-functions"
      else
	AC_MSG_RESULT([no])
      fi
    done
    ;;
esac

# Substitute

AC_SUBST([XORG_CFLAGS])
AC_SUBST([WARN_CFLAGS])
AC_SUBST([moduledir])

DRIVER_NAME=radeonhd
AC_SUBST([DRIVER_NAME])

# XORG_MANPAGE_SECTIONS
XORG_RELEASE_VERSION

dnl These are also defined in XORG_RELEASE_VERSION since 2006-11-30.
dnl So we check whether XORG_RELEASE_VERSION has already done this
dnl for us.
if test "x$PVM$PVP" = "x"; then
AC_DEFINE_UNQUOTED([PACKAGE_VERSION_MAJOR],
		[$(echo $PACKAGE_VERSION | cut -d . -f 1)],
		[Major Version])
AC_DEFINE_UNQUOTED([PACKAGE_VERSION_MINOR],
		[$(echo $PACKAGE_VERSION | cut -d . -f 2)],
		[Minor Version])
AC_DEFINE_UNQUOTED([PACKAGE_VERSION_PATCHLEVEL],
		[$(echo $PACKAGE_VERSION | cut -d . -f 3)],
		[Patchlevel Version])
fi

AC_OUTPUT([
	Makefile
	src/Makefile
])
