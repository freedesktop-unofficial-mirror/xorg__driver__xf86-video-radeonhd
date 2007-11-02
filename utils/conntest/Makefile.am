EXTRA_DIST = README Imakefile

noinst_PROGRAMS = rhd_conntest

AM_CFLAGS   = @WARN_CFLAGS@
AM_CPPFLAGS = -I$(top_srcdir)/src/AtomBios/includes

rhd_conntest_SOURCES = rhd_conntest.c
rhd_conntest_LDADD   = -lz -lpci
