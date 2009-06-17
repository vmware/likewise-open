noinst_LTLIBRARIES = libsrvelements.la

libsrvelements_la_SOURCES = \
    globals.c       \
    libmain.c       \
    srvconnection.c \
    srvfile.c       \
    srvfinder.c     \
    srvgss.c        \
    srvsession.c    \
    srvtimer.c      \
    srvtree.c

libsrvelements_la_CPPFLAGS = \
    -DLW_ENABLE_THREADS=1  \
    -I@top_srcdir@         \
    -I@top_srcdir@/include \
    -I@top_srcdir@/server/include \
    -I@top_srcdir@/server/srv/include

libsrvelements_la_LDFLAGS = -static

