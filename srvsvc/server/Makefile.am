sbin_PROGRAMS = srvsvcd

lib_LTLIBRARIES = libsrvsvc_cs.la libsrvsvc_ss.la

idl_srcdir=$(top_srcdir)/idl
include_srcdir=$(top_srcdir)/include

srvsvcd_SOURCES =  \
    driver.c       \
    globals.c      \
    main.c         \
    marshall.c     \
    memutil.c      \
    server.c       \
    srvsvc_server.c

libsrvsvc_cs_la_SOURCES = srvsvc_cstub.c
libsrvsvc_ss_la_SOURCES = srvsvc_sstub.c

libsrvsvc_cs_la_CPPFLAGS = @DCERPC_INCLUDES@
libsrvsvc_ss_la_CPPFLAGS = @DCERPC_INCLUDES@

srvsvcd_CFLAGS=-g $(INCLUDES) -DSRVSVC_BUILD
srvsvcd_CPPFLAGS = \
    @DCERPC_INCLUDES@   \
    @LIBUNISTR_INCLUDES@ \
    @LWIO_INCLUDES@

srvsvcd_LDFLAGS = $(DCERPC_LDFLAGS) $(UNISTR_LDFLAGS) @LWIO_LDFLAGS@
srvsvcd_LDADD = \
    @top_builddir@/utils/libsrvsvcutils.la \
    libsrvsvc_ss.la \
    -ldcerpc -lunistr @LWIO_LIBS@

CLEANFILES = srvsvc_h.h srvsvc_cstub.c srvsvc_sstub.c

BUILT_SOURCES = srvsvc_h.h srvsvc_cstub.c srvsvc_sstub.c

srvsvc_h.h srvsvc_cstub.c srvsvc_sstub.c: $(idl_srcdir)/srvsvc.idl
	$(IDL) $(IDLFLAGS) -keep c_source -header srvsvc_h.h -I$(idl_srcdir) -I$(include_srcdir) $<

