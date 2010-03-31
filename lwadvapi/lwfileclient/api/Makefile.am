lib_LTLIBRARIES = liblwfileclient.la

liblwfileclient_la_SOURCES = \
    fileclient.c        \
    fileio.c

liblwfileclient_la_CPPFLAGS = \
    -I@top_srcdir@/include\
    -I@top_srcdir@/utils \
    @LWADVAPI_INCLUDES@ \
    @LWIO_INCLUDES@ \
    $(AM_CPPFLAGS)

liblwfileclient_la_LIBADD = \
    @top_builddir@/utils/liblwfileclientutils.la \
    @LWADVAPI_LIBS@ \
    @LWIO_LIBS@ \
    @RESOLV_LIBS@ \
    @NSL_LIBS@  \
    @RT_LIBS@

liblwfileclient_la_LDFLAGS = \
    -module -avoid-version -export-dynamic \
    @LWIO_LDFLAGS@

noinst_HEADERS = \
    fileio.h
