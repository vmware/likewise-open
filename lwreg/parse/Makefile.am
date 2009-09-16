lib_LTLIBRARIES = libregparse.la

libregparse_la_SOURCES = \
    regio.c \
    reglex.c \
    regiconv.c \
    regparse.c \
    regdump.c

libregparse_la_CPPFLAGS = \
    $(AM_CFLAGS)          \
    -I@top_srcdir@/server/include \
    @LWADVAPI_INCLUDES@   \
    @LWMSG_INCLUDES@

libregparse_la_LIBADD = \
        @top_builddir@/utils/libregutils.la \
        @LWADVAPI_NOTHR_LIBS@ \
        @LWMSG_NOTHR_LIBS@

libregparse_la_LDFLAGS = @LWADVAPI_LDFLAGS@
