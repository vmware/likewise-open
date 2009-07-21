librpc_srcdir=$(top_srcdir)
idl_srcdir=$(top_srcdir)/idl

noinst_LTLIBRARIES = libnetlogon_cs.la libnetlogon.la

libnetlogon_la_SOURCES = \
    globals.c                         \
    netr_binding.c                    \
    netr_memory.c                     \
    netr_stubmemory.c                 \
    netr_credentials.c                \
    netr_serverreqchallenge.c         \
    netr_serverauthenticate.c         \
    netr_serverauthenticate2.c        \
    netr_serverauthenticate3.c        \
    netr_schannel.c                   \
    netr_samlogon.c                   \
    netr_samlogoff.c                  \
    netr_samlogonex.c                 \
    netr_enumeratetrusteddomainsex.c  \
    netr_getdomaininfo.c              \
    dsr_enumeratedomaintrusts.c       \
    dsr_getdcname.c

libnetlogon_cs_la_SOURCES = netlogon_cstub.c

LIBRPC_INCDIR = $(librpc_srcdir)/include
LIBRPC_INCLUDES = -I$(LIBRPC_INCDIR)
INCLUDES = $(DCERPC_INCLUDES) $(UNISTR_INCLUDES) $(LIBRPC_INCLUDES)

FEATURETEST_FLAGS="-D_XOPEN_SOURCE=500 -D__EXTENSIONS__ -D_REENTRANT"

libnetlogon_cs_la_CPPFLAGS = $(FEATURETEST_FLAGS)
libnetlogon_cs_la_CFLAGS = \
    -DDCERPC_STUB_BUILD \
    $(FEATURETEST_FLAGS)

libnetlogon_la_CFLAGS = -g $(INCLUDES) -DLIBRPC_BUILD $(FEATURETEST_FLAGS) $(AM_CFLAGS)
libnetlogon_la_LDFLAGS = $(DCERPC_LDFLAGS) $(UNISTR_LDFLAGS)
libnetlogon_la_LIBADD = \
    -ldcerpc -lunistr -llwbase \
    libnetlogon_cs.la

CLEANFILES = netlogon_h.h netlogon_cstub.c netlogon_sstub.c

BUILT_SOURCES = netlogon_h.h

netlogon_cstub.c netlogon_h.h: $(idl_srcdir)/netlogon.idl
	$(IDL) $(IDLFLAGS) -keep c_source -header netlogon_h.h -I$(idl_srcdir) \
		$(LIBRPC_INCLUDES) $<
