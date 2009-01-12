lib_LTLIBRARIES = libsmbkrb5.la

libsmbkrb5_la_SOURCES = \
    globals.c \
    smbkrb5.c

libsmbkrb5_la_CPPFLAGS = \
    $(AM_CPPFLAGS) \
    -I@top_srcdir@/include

libsmbkrb5_la_LIBADD = \
    @top_builddir@/utils/libsmbutils.la \
    @KRB5_LIBS@
