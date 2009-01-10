lib_LTLIBRARIES = liblwiosmbkrb5.la

liblwiosmbkrb5_la_SOURCES = \
    globals.c \
    smbkrb5.c

liblwiosmbkrb5_la_CPPFLAGS = \
    $(AM_CPPFLAGS) \
    -I@top_srcdir@/include

liblwiosmbkrb5_la_LIBADD = \
    @top_builddir@/utils/liblwioutils.la \
    @KRB5_LIBS@
