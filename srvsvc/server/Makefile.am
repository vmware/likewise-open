AM_CPPFLAGS = -I$(top_srcdir)/include

lib_LTLIBRARIES = libsrvsvc_ss.la libwinreg_ss.la

idl_srcdir = $(top_srcdir)/idl
include_srcdir = $(top_srcdir)/include

libsrvsvc_ss_la_SOURCES = srvsvc_sstub.c
libwinreg_ss_la_SOURCES = winreg_sstub.c

libsrvsvc_ss_la_CPPFLAGS = \
    $(AM_CPPFLAGS)         \
    -DLW_ENABLE_THREADS=1  \
    -DDCERPC_STUB_BUILD    \
    $(DCERPC_INCLUDES)     \
    $(LSASS_INCLUDES)

libsrvsvc_ss_la_CFLAGS = \
    $(AM_CFLAGS)       \
    -DDCERPC_STUB_BUILD  \
    $(DCERPC_INCLUDES)

libwinreg_ss_la_CPPFLAGS = \
    $(AM_CPPFLAGS)         \
    -DDCERPC_STUB_BUILD    \
    $(DCERPC_INCLUDES)     \
    $(LSASS_INCLUDES)

libwinreg_ss_la_CFLAGS = \
    $(AM_CFLAGS)       \
    -DDCERPC_STUB_BUILD  \
    $(DCERPC_INCLUDES)

CLEANFILES = srvsvc_h.h srvsvc_cstub.c srvsvc_sstub.c \
             winreg_h.h winreg_cstub.c winreg_sstub.c

BUILT_SOURCES = \
    srvsvc_h.h  \
    winreg_h.h

srvsvc_h.h srvsvc_cstub.c srvsvc_sstub.c: $(idl_srcdir)/srvsvc.idl $(idl_srcdir)/srvsvc.acf
	$(IDL) $(IDLFLAGS) -keep c_source -header srvsvc_h.h \
		-I$(srcdir) -I$(idl_srcdir) -I$(include_srcdir) $<

winreg_h.h winreg_cstub.c winreg_sstub.c: $(idl_srcdir)/winreg.idl $(idl_srcdir)/winreg.acf
	$(IDL) $(IDLFLAGS) -keep c_source -header winreg_h.h \
		-I$(srcdir) -I$(idl_srcdir) -I$(include_srcdir) $<

SRVSVC_CPPFLAGS = \
    $(AM_CPPFLAGS)        \
    $(DCERPC_INCLUDES)    \
    $(LSASS_INCLUDES)     \
    $(LIBUNISTR_INCLUDES) \
    $(LWADVAPI_INCLUDES)  \
    $(LWIO_INCLUDES)      \
    $(NETLOGON_INCLUDES)

SRVSVC_LDFLAGS = \
    $(DCERPC_LDFLAGS)    \
    $(LIBUNISTR_LDFLAGS) \
    $(LWADVAPI_LDFLAGS)  \
    $(LWIO_LDFLAGS)      \
    $(NETLOGON_LDFLAGS)  \
    $(DS_FRAMEWORK_LDFLAGS)

SRVSVC_LIBS = \
    $(top_builddir)/utils/libsrvsvcutils.la \
    libsrvsvc_ss.la                         \
    libwinreg_ss.la                         \
    $(LSASS_LIBS)                           \
    $(DCERPC_LIBS)                          \
    $(LWIO_LIBS)                            \
    -llwbase                                \
    -llwbase_nothr                          \
    -llwadvapi                              \
    -llwadvapi_nothr

svcmdir = $(libdir)/lw-svcm

svcm_LTLIBRARIES = srvsvc.la

srvsvc_la_SOURCES =            \
    accesstoken.c              \
    config.c                   \
    netfile.c                  \
    netsession.c               \
    srvsvc.c                   \
    srvsvc_srv.c               \
    winreg.c                   \
    winreg_srv.c               \
    globals.c                  \
    marshall.c                 \
    security.c                 \
    srvsvc_netshareadd.c       \
    srvsvc_netsharedel.c       \
    srvsvc_netsharegetinfo.c   \
    srvsvc_netsharesetinfo.c   \
    srvsvc_netshareenum.c      \
    srvsvc_netservergetinfo.c  \
    srvsvc_netnamevalidate.c   \
    srvsvc_netconnectionenum.c \
    utils.c                    \
    main-svcm.c

srvsvc_la_CPPFLAGS = $(SRVSVC_CPPFLAGS)

srvsvc_la_LIBADD = $(SRVSVC_LIBS)

srvsvc_la_LDFLAGS = $(SRVSVC_LDFLAGS) -module -avoid-version
