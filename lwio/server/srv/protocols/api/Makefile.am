noinst_LTLIBRARIES = libsrv.protocols.api.la

libsrv_protocols_api_la_SOURCES = \
    config.c    \
    globals.c   \
    libmain.c   \
    negotiate.c \
    worker.c

libsrv_protocols_api_la_CPPFLAGS = \
    -DLW_ENABLE_THREADS=1  \
    -I@top_srcdir@         \
    -I@top_srcdir@/include \
    -I@top_srcdir@/server/include \
    -I@top_srcdir@/server/srv/include \
    -I@top_srcdir@/server/srv/protocols/include

libsrv_protocols_api_la_LIBADD = \
    @top_builddir@/server/srv/protocols/smb1/libsrv.protocols.smb1.la \
    @top_builddir@/server/srv/protocols/smb2/libsrv.protocols.smb2.la

libsrv_protocols_api_la_LDFLAGS = -static

