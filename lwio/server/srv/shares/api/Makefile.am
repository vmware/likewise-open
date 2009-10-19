noinst_LTLIBRARIES = libshareapi.la

libshareapi_la_SOURCES = \
    globals.c  \
    libmain.c  \
    mapping.c  \
    shareapi.c \
    utils.c

libshareapi_la_CPPFLAGS = \
    -DLW_ENABLE_THREADS=1  \
    @SHARE_REPOSITORY_TYPE_DEFINES@ \
    -I@top_srcdir@         \
    -I@top_srcdir@/include \
    -I@top_srcdir@/server/include \
    -I@top_srcdir@/server/srv/include \
    -I@top_srcdir@/server/srv/shares/include

if ENABLE_SHARE_REPOSITORY_SQLITE

libshareapi_la_LIBADD = \
    @top_builddir@/server/srv/shares/lwshare/liblwshare.la

endif

if ENABLE_SHARE_REPOSITORY_REGISTRY

libshareapi_la_LIBADD = \
    @top_builddir@/server/srv/shares/regshare/liblwregshare.la

endif

libshareapi_la_LDFLAGS = -static

