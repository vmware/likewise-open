lib_LTLIBRARIES = libregutils.la

libregutils_la_SOURCES = \
    consolelog.c         \
    filelog.c            \
    fileutils.c          \
    globals.c            \
    reglogger.c          \
    regloginfo.c         \
    regmem.c             \
    sysfuncs.c           \
    syslog.c             \
    regtime.c            \
    dlinkedlist.c        \
    hashtable.c          \
    regerror.c           \
    regdatatype.c
    
noinst_HEADERS = \
    externs.h       \
    includes.h

libregutils_la_CPPFLAGS = $(AM_CPPFLAGS) @LWBASE_INCLUDES@ @LWADVAPI_INCLUDES@ \
                          -I@top_srcdir@/server/include 
libregutils_la_LDFLAGS = @LWBASE_LDFLAGS@ \
                         @LWADVAPI_LDFLAGS@
libregutils_la_LIBADD = @LWBASE_NOTHR_LIBS@ \
                        @LWADVAPI_NOTHR_LIBS@
