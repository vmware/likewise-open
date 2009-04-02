provider_LTLIBRARIES = libdsapi.la

libdsapi_la_SOURCES = \
        diradd.c      \
        dirattr.c     \
        dirbind.c     \
        dirclose.c    \
        dirdel.c      \
        dirglobals.c  \
        dirgroup.c    \
        dirmem.c      \
        dirmodify.c   \
        diropen.c     \
        dirprovider.c \
        dirsearch.c   \
        diruser.c

libdsapi_la_CPPFLAGS = \
	-DLW_ENABLE_THREADS=1 \
	-I@top_srcdir@/server/include \
    -I@top_srcdir@/server/store/include

libdsapi_la_LIBADD = \
	@LWBASE_LIBS@

libdsapi_la_LDFLAGS = \
	@LWBASE_LDFLAGS@


