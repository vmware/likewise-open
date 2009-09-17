lib_LTLIBRARIES = librsutils.la

librsutils_la_SOURCES = \
    rsutils.c \
    import.c

librsutils_la_CPPFLAGS = \
    $(AM_CFLAGS)          \
    -I@top_srcdir@/server/include \
    @LWADVAPI_INCLUDES@   \
    @LWMSG_INCLUDES@

librsutils_la_LIBADD = \
        @top_builddir@/utils/libregutils.la \
        @LWADVAPI_NOTHR_LIBS@ \
        @LWMSG_NOTHR_LIBS@

librsutils_la_LDFLAGS = @LWADVAPI_LDFLAGS@
