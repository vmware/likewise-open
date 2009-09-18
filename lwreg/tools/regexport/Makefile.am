AM_CPPFLAGS = -I$(top_srcdir)/include

bin_PROGRAMS = regexport

regexport_CPPFLAGS = \
    -I@top_srcdir@/server/include \
    -I@top_builddir@ \
    $(AM_CPPFLAGS)

regexport_LDADD = \
    @top_builddir@/client/libregclient.la        \
    @top_builddir@/parse/libregparse.la

regexport_SOURCES = main.c