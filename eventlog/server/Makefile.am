sbin_PROGRAMS = eventlogd

idl_srcdir=$(top_srcdir)/idl

eventlogd_SOURCES = \
    db.c             \
    eventlog_sstub.c \
    globals.c        \
    main.c           \
    security.c       \
    server.c

eventlogd_CPPFLAGS = \
    @GSSAPI_INCLUDES@ \
    @DCERPC_INCLUDES@ \
    @LIBUNISTR_INCLUDES@

eventlogd_LDADD = \
    @top_builddir@/utils/libeventlogutils.la \
    @GSSAPI_LIBS@ \
    @SQLITE_LIBS@

eventlogd_LDFLAGS = \
    @GSSAPI_LDFLAGS@ \
    @SQLITE_LDFLAGS@
    
CLEANFILES = \
    eventlog_h.h \
    eventlog_cstub.c \
    eventlog_sstub.c

BUILT_SOURCES = eventlog_h.h

eventlog_h.h eventlog_sstub.c: $(idl_srcdir)/eventlog.idl
	$(IDL) $(IDLFLAGS) -keep c_source -header eventlog_h.h -I$(idl_srcdir) $<

