noinst_LTLIBRARIES = libsrvelements.la

libsrvelements_la_SOURCES = \
    libmain.c    \
    srvsession.c \
    srvtree.c    \
    srvfile.c

libsrvelements_la_CPPFLAGS = \
    -DLW_ENABLE_THREADS=1  \
    -I@top_srcdir@         \
    -I@top_srcdir@/include \
    -I@top_srcdir@/server/include \
    -I@top_srcdir@/server/srv.new/include

libsrvelements_la_LDFLAGS = -static

