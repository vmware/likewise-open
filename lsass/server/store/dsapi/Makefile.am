provider_LTLIBRARIES = libdsapi.la

libdsapi_la_SOURCES = \
        diradd.c    \
        dirbind.c   \
        dirclose.c  \
        dirdel.c    \
        dirmem.c    \
        dirmodify.c \
        diropen.c   \
        dirsearch.c

libdsapi_la_CPPFLAGS = \
	-DLW_ENABLE_THREADS=1         \
	-I@top_srcdir@/server/include

libdsapi_la_LDFLAGS = \
	-module -avoid-version -export-dynamic
