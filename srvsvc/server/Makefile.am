sbin_PROGRAMS = srvsvcd

lib_LTLIBRARIES = libsrvsvc_ss.la libwkssvc_ss.la

idl_srcdir=$(top_srcdir)/idl
include_srcdir=$(top_srcdir)/include

srvsvcd_SOURCES =              \
    srvsvc.c                   \
    srvsvc_srv.c               \
    wkssvc.c                   \
    wkssvc_srv.c               \
    globals.c                  \
    marshall.c                 \
    memutil.c                  \
    srvsvc_netshareadd.c       \
    srvsvc_netsharedel.c       \
    srvsvc_netsharegetinfo.c   \
    srvsvc_netsharesetinfo.c   \
    srvsvc_netshareenum.c      \
    srvsvc_netservergetinfo.c  \
    srvsvc_netnamevalidate.c   \
    wkssvc_netwkstagetinfo.c   \
    main.c


libsrvsvc_ss_la_SOURCES = srvsvc_sstub.c
libwkssvc_ss_la_SOURCES = wkssvc_sstub.c

FEATURETEST_FLAGS="-D_XOPEN_SOURCE=500"

libsrvsvc_ss_la_CPPFLAGS = \
    $(FEATURETEST_FLAGS) \
    @DCERPC_INCLUDES@

libwkssvc_ss_la_CFLAGS = \
    $(FEATURETEST_FLAGS) \
    @DCERPC_INCLUDES@

libwkssvc_ss_la_CPPFLAGS = \
    $(FEATURETEST_FLAGS) \
    @DCERPC_INCLUDES@

libsrvsvc_ss_la_CFLAGS = \
    $(FEATURETEST_FLAGS) \
    @DCERPC_INCLUDES@

srvsvcd_CFLAGS = -g $(INCLUDES) -DSRVSVC_BUILD
srvsvcd_CPPFLAGS = \
    @DCERPC_INCLUDES@   \
    @LIBUNISTR_INCLUDES@ \
    @LWIO_INCLUDES@

srvsvcd_LDFLAGS = $(DCERPC_LDFLAGS) $(UNISTR_LDFLAGS) @LWIO_LDFLAGS@
srvsvcd_LDADD = \
    @top_builddir@/utils/libsrvsvcutils.la \
    libsrvsvc_ss.la \
    libwkssvc_ss.la \
    -ldcerpc -lunistr @LWIO_LIBS@

CLEANFILES = srvsvc_h.h srvsvc_cstub.c srvsvc_sstub.c \
             wkssvc_h.h wkssvc_cstub.c wkssvc_sstub.c

BUILT_SOURCES = srvsvc_h.h srvsvc_cstub.c srvsvc_sstub.c \
                wkssvc_h.h wkssvc_cstub.c wkssvc_sstub.c

srvsvc_h.h srvsvc_cstub.c srvsvc_sstub.c: $(idl_srcdir)/srvsvc.idl
	$(IDL) $(IDLFLAGS) -keep c_source -header srvsvc_h.h -I$(idl_srcdir) -I$(include_srcdir) $<

wkssvc_h.h wkssvc_cstub.c wkssvc_sstub.c: $(idl_srcdir)/wkssvc.idl
	$(IDL) $(IDLFLAGS) -keep c_source -header wkssvc_h.h -I$(idl_srcdir) -I$(include_srcdir) $<

