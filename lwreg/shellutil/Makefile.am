lib_LTLIBRARIES = librsutils.la

librsutils_la_SOURCES = \
    regutilwrap.c \
    rsutils.c \
    import.c \
    export.c

librsutils_la_CPPFLAGS = \
    $(AM_CPPFLAGS)          \
    -I@top_srcdir@/server/include \
    @LWADVAPI_INCLUDES@   \
    @LWMSG_INCLUDES@

librsutils_la_LIBADD = \
        @top_builddir@/utils/libregutils.la \
        @LWADVAPI_NOTHR_LIBS@ \
        @LWMSG_NOTHR_LIBS@

librsutils_la_LDFLAGS = @LWADVAPI_LDFLAGS@
