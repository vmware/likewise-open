provider_LTLIBRARIES = libsamdb.la

libsamdb_la_SOURCES =  \
        samdbadd.c     \
        samdbbind.c    \
        samdbclose.c   \
        samdbcontext.c \
        samdbdel.c     \
        samdbglobals.c \
        samdbgroup.c   \
        samdbinit.c    \
        samdblock.c    \
        samdbmisc.c    \
        samdbmodify.c  \
        samdbopen.c    \
        samdbsearch.c  \
        samdbuser.c

libsamdb_la_CPPFLAGS = \
	-DLW_ENABLE_THREADS=1         \
	-I@top_srcdir@/server/include \
        -I@top_srcdir@/server/rpc/include

libsamdb_la_LDFLAGS = \
	-module -avoid-version -export-dynamic -export-symbols=samdb.sym
