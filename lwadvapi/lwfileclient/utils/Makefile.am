lib_LTLIBRARIES = liblwfileclientutils.la

liblwfileclientutils_la_SOURCES = \
    fileclientutils.c

libilwfileclintutils_la_CPPFLAGS = \
    -I$(top_srcdir)/include \
    -I@top_srcdir@/utils \
    @LWADVAPI_INCLUDES@ \
    @LWIO_INCLUDES@ \
    $(AM_CPPFLAGS)

liblwfileclientutils_la_LIBADD = \
    @LWADVAPI_LIBS@ \
    @LWIO_LIBS@ \
    @RESOLV_LIBS@ \
    @NSL_LIBS@  \
    @RT_LIBS@

