AM_CPPFLAGS = -I$(top_srcdir)/include

sbin_PROGRAMS = srvsvcd

lib_LTLIBRARIES = libsrvsvc_ss.la libwkssvc_ss.la libwinreg_ss.la

idl_srcdir = $(top_srcdir)/idl
include_srcdir = $(top_srcdir)/include

srvsvcd_SOURCES =              \
    srvsvc.c                   \
    srvsvc_srv.c               \
    wkssvc.c                   \
    wkssvc_srv.c               \
    winreg.c                   \
    winreg_srv.c               \
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
libwinreg_ss_la_SOURCES = winreg_sstub.c

FEATURETEST_FLAGS="-D_XOPEN_SOURCE=500"

libsrvsvc_ss_la_CPPFLAGS = \
    -DDCERPC_STUB_BUILD    \
    $(FEATURETEST_FLAGS)   \
    $(DCERPC_INCLUDES)

libsrvsvc_ss_la_CFLAGS = \
    -DDCERPC_STUB_BUILD  \
    $(FEATURETEST_FLAGS) \
    $(DCERPC_INCLUDES)

libwkssvc_ss_la_CPPFLAGS = \
    -DDCERPC_STUB_BUILD    \
    $(FEATURETEST_FLAGS)   \
    $(DCERPC_INCLUDES)

libwkssvc_ss_la_CFLAGS = \
    -DDCERPC_STUB_BUILD  \
    $(FEATURETEST_FLAGS) \
    $(DCERPC_INCLUDES)

libwinreg_ss_la_CPPFLAGS = \
    -DDCERPC_STUB_BUILD    \
    $(FEATURETEST_FLAGS)   \
    $(DCERPC_INCLUDES)

libwinreg_ss_la_CFLAGS = \
    -DDCERPC_STUB_BUILD  \
    $(FEATURETEST_FLAGS) \
    $(DCERPC_INCLUDES)


srvsvcd_CFLAGS = \
    -DSRVSVC_BUILD   \
    -DLIBRPC_BUILD

srvsvcd_CPPFLAGS = \
    $(AM_CPPFLAGS)        \
    $(DCERPC_INCLUDES)    \
    $(LIBRPC_INCLUDES)    \
    $(LIBUNISTR_INCLUDES) \
    $(LWADVAPI_INCLUDES)  \
    $(LWIO_INCLUDES)      \
    $(NETLOGON_INCLUDES)

srvsvcd_LDFLAGS = \
    $(DCERPC_LDFLAGS)    \
    $(LIBRPC_LDFLAGS)    \
    $(LIBUNISTR_LDFLAGS) \
    $(LWADVAPI_LDFLAGS)  \
    $(LWIO_LDFLAGS)      \
    $(NETLOGON_LDFLAGS)

srvsvcd_LDADD = \
    $(top_builddir)/utils/libsrvsvcutils.la \
    libsrvsvc_ss.la                         \
    libwkssvc_ss.la                         \
    libwinreg_ss.la                         \
    $(DCERPC_LIBS)                          \
    $(LIBRPC_LIBS)                          \
    $(LIBUNISTR_LIBS)                       \
    $(LWADVAPI_LIBS)                        \
    $(LWIO_LIBS)                            \
    $(NETLOGON_LIBS)

CLEANFILES = srvsvc_h.h srvsvc_cstub.c srvsvc_sstub.c \
             wkssvc_h.h wkssvc_cstub.c wkssvc_sstub.c \
             winreg_h.h winreg_cstub.c winreg_sstub.c

BUILT_SOURCES = srvsvc_h.h srvsvc_cstub.c srvsvc_sstub.c \
                wkssvc_h.h wkssvc_cstub.c wkssvc_sstub.c \
                winreg_h.h winreg_cstub.c winreg_sstub.c

srvsvc_h.h srvsvc_cstub.c srvsvc_sstub.c: $(idl_srcdir)/srvsvc.idl $(idl_srcdir)/srvsvc.acf
	$(IDL) $(IDLFLAGS) -keep c_source -header srvsvc_h.h \
		-I$(srcdir) -I$(idl_srcdir) -I$(include_srcdir) $<

wkssvc_h.h wkssvc_cstub.c wkssvc_sstub.c: $(idl_srcdir)/wkssvc.idl $(idl_srcdir)/wkssvc.acf
	$(IDL) $(IDLFLAGS) -keep c_source -header wkssvc_h.h \
		-I$(srcdir) -I$(idl_srcdir) -I$(include_srcdir) $<

winreg_h.h winreg_cstub.c winreg_sstub.c: $(idl_srcdir)/winreg.idl $(idl_srcdir)/winreg.acf
	$(IDL) $(IDLFLAGS) -keep c_source -header winreg_h.h \
		-I$(srcdir) -I$(idl_srcdir) -I$(include_srcdir) $<
