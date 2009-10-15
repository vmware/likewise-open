noinst_LTLIBRARIES = liblwregshare.la

liblwregshare_la_SOURCES = \
    globals.c   \
    libmain.c   \
    regshare.c

liblwregshare_la_CPPFLAGS = \
    -DLW_ENABLE_THREADS=1  \
    -I@top_srcdir@         \
    -I@top_srcdir@/include \
    -I@top_srcdir@/server/include \
    -I@top_srcdir@/server/srv/include \
    -I@top_srcdir@/server/srv/shares/include

liblwregshare_la_LIBADD = \
    @SQLITE_LIBS@

liblwregshare_la_LDFLAGS = -static

