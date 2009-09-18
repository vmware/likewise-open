AM_CPPFLAGS = -I$(top_srcdir)/include

bin_PROGRAMS = regimport

regimport_CPPFLAGS = \
    -I@top_srcdir@/server/include \
    -I@top_builddir@ \
    $(AM_CPPFLAGS)

regimport_LDADD = \
    @top_builddir@/shellutil/librsutils.la       \
    @top_builddir@/client/libregclient.la        \
    @top_builddir@/parse/libregparse.la

regimport_SOURCES = main.c
