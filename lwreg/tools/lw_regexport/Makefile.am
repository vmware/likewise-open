AM_CPPFLAGS = -I$(top_srcdir)/include

bin_PROGRAMS = lw_regexport

lw_regexport_CPPFLAGS = \
    -I@top_srcdir@/server/include \
    -I@top_builddir@ \
    $(AM_CPPFLAGS)

lw_regexport_LDADD = \
    @top_builddir@/client/libregclient.la        \
    @top_builddir@/parse/libregparse.la

lw_regexport_SOURCES = main.c