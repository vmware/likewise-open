AM_CPPFLAGS = -I$(top_srcdir)/include

bin_PROGRAMS = lw_regimport

lw_regimport_CPPFLAGS = \
    -I@top_srcdir@/server/include \
    -I@top_builddir@ \
    $(AM_CPPFLAGS)

lw_regimport_LDADD = \
    @top_builddir@/shellutil/librsutils.la       \
    @top_builddir@/client/libregclient.la        \
    @top_builddir@/parse/libregparse.la

lw_regimport_SOURCES = main.c
