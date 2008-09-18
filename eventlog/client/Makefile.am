lib_LTLIBRARIES = libeventlog.la

idl_srcdir=$(top_srcdir)/idl

libeventlog_la_SOURCES = \
    binding.c  \
    eventlog.c \
    eventlog_cstub.c

libeventlog_la_LIBADD = \
    @top_builddir@/utils/libeventlogutils.la

libeventlog_la_CPPFLAGS = \
    @DCERPC_INCLUDES@   \
    @LIBUNISTR_INCLUDES@

CLEANFILES = eventlog_h.h eventlog_cstub.c eventlog_sstub.c

BUILT_SOURCES = eventlog_h.h

eventlog_h.h eventlog_cstub.c: $(idl_srcdir)/eventlog.idl
	$(IDL) $(IDLFLAGS) -keep c_source -header eventlog_h.h -I$(idl_srcdir) $<

