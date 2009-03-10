provider_LTLIBRARIES = libdirapi.la

libdirapi_la_SOURCES = \
        diradd.c    \
        dirbind.c   \
        dirclose.c  \
        dirdel.c    \
        dirmodify.c \
        diropen.c   \
        dirsearch.c

libdirapi_la_CPPFLAGS = \
	-DLW_ENABLE_THREADS=1         \
	-I@top_srcdir@/server/include \
	-I@top_srcdir@/server/rpc/include

libdirapi_la_LDFLAGS = \
	-module -avoid-version -export-dynamic
