lib_LTLIBRARIES = libregserver_provider_sqlite.la

PROVIDER_CPPFLAGS = @SQLITE_INCLUDES@
PROVIDER_LIBADD = @SQLITE_LIBS@
PROVIDER_LDFLAGS = @SQLITE_LDFLAGS@

libregserver_provider_sqlite_la_SOURCES = \
    sqliteapi.c    \
    sqliteapi_p.c    \
    sqldb.c     \
    regsqlite.c    \
    globals.c           

libregserver_provider_sqlite_la_CPPFLAGS =        \
    -DLW_ENABLE_THREADS=1         \
    $(AM_CPPFLAGS)                \
    -I@top_srcdir@/server/include \
    @LWADVAPI_INCLUDES@ \
    $(PROVIDER_CPPFLAGS)          

libregserver_provider_sqlite_la_LIBADD = \
    @PTHREAD_LIBS@    \
    @UUID_LIBS@ \
    @LWADVAPI_LIBS@ \
    @LWBASE_LIBS@ \
    $(PROVIDER_LIBADD)

libregserver_provider_sqlite_la_LDFLAGS = \
    @LWADVAPI_LDFLAGS@ \
    $(PROVIDER_LDFLAGS)