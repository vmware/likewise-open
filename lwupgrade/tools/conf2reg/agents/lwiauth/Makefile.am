noinst_LTLIBRARIES = libconflwiauth.la

libconflwiauth_la_SOURCES = \
    lwiauth.c \
    readpasswd4.c \
    tdb.c

libconflwiauth_la_CPPFLAGS = \
    $(AM_CPPFLAGS) \
    -I$(top_builddir)/include

libconflwiauth_la_LIBADD = \
    @TDB_LIBS@ \
    @LIBRPC_LIBS@
