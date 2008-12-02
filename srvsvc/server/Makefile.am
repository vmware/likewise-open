
sbin_PROGRAMS = srvsvcd

idl_srcdir=$(top_srcdir)/idl
include_srcdir=$(top_srcdir)/include

srvsvcd_SOURCES = \
    main.c \
    srvsvc_sstub.c \
    srvsvc_server.c

srvsvcd_CFLAGS=-g $(INCLUDES) -DSRVSVC_BUILD
srvsvcd_CPPFLAGS = \
    @DCERPC_INCLUDES@   \
    @LIBUNISTR_INCLUDES@

srvsvcd_LDFLAGS=$(DCERPC_LDFLAGS) $(UNISTR_LDFLAGS)
srvsvcd_LDADD=-ldcerpc -lunistr

CLEANFILES = srvsvc_h.h srvsvc_cstub.c srvsvc_sstub.c

BUILT_SOURCES = srvsvc_h.h

srvsvc_h.h srvsvc_sstub.c: $(idl_srcdir)/srvsvc.idl
	$(IDL) $(IDLFLAGS) -keep c_source -header srvsvc_h.h -I$(idl_srcdir) -I$(include_srcdir) $<

