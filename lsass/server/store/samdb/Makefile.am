provider_LTLIBRARIES = libsamdb.la

libsamdb_la_SOURCES =     \
        samdbadd.c        \
        samdbattrlookup.c \
        samdbbind.c       \
        samdbclose.c      \
        samdbcontext.c    \
        samdbcounter.c    \
        samdbdel.c        \
        samdbdn.c         \
        samdbglobals.c    \
        samdbinit.c       \
        samdblock.c       \
        samdbmisc.c       \
        samdbmodify.c     \
        samdbopen.c       \
        samdbschema.c     \
        samdbsearch.c     \
        samdbtrans.c      \
        samdbuser.c

libsamdb_la_CPPFLAGS = \
	-DLW_ENABLE_THREADS=1         \
	-I@top_srcdir@/server/include \
	-I@top_srcdir@/server/store/include

libsamdb_la_LIBADD = \
    @UUID_LIBS@ \
    @top_builddir@/server/store/dsapi/libdsapi.la

libsamdb_la_LDFLAGS = \
	-module -avoid-version -export-dynamic -export-symbols=samdb.sym
