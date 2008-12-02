lib_LTLIBRARIES = libsrvsvc.la

idl_srcdir=$(top_srcdir)/idl
include_srcdir=$(top_srcdir)/include

libsrvsvc_la_SOURCES = \
    SrvSvcBinding.c  \
    srvsvc_cstub.c   \
    SrvSvcNetConnectionEnum.c \
    SrvSvcNetFileEnum.c \
    SrvSvcNetFileGetInfo.c \
    SrvSvcNetFileClose.c \
    SrvSvcNetSessionEnum.c \
    SrvSvcNetShareAdd.c \
    SrvSvcNetShareEnum.c \
    SrvSvcNetShareGetInfo.c \
    SrvSvcNetShareSetInfo.c \
    SrvSvcNetShareDel.c \
    SrvSvcNetServerGetInfo.c \
    SrvSvcNetServerSetInfo.c \
    SrvSvcNetRemoteTOD.c \
    SrvSvcSecurity.c \
    SrvSvcMemory.c \
    SrvSvcStubMemory.c \
    MemPtr.c \
    globals.c

libsrvsvc_la_CFLAGS=-g $(INCLUDES) -DSRVSVC_BUILD
libsrvsvc_la_CPPFLAGS = \
    @DCERPC_INCLUDES@   \
    @LIBUNISTR_INCLUDES@

libsrvsvc_la_LDFLAGS=$(DCERPC_LDFLAGS) $(UNISTR_LDFLAGS)
libsrvsvc_la_LIBADD=-ldcerpc -lunistr

CLEANFILES = srvsvc_h.h srvsvc_cstub.c srvsvc_sstub.c

BUILT_SOURCES = srvsvc_h.h

srvsvc_h.h srvsvc_cstub.c: $(idl_srcdir)/srvsvc.idl
	$(IDL) $(IDLFLAGS) -keep c_source -header srvsvc_h.h -I$(idl_srcdir) -I$(include_srcdir) $<
