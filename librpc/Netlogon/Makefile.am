librpc_srcdir=$(top_srcdir)
idl_srcdir=$(top_srcdir)/Idl

lib_LTLIBRARIES = libnetlogon_cs.la libnetlogon.la

libnetlogon_la_SOURCES = \
	globals.c \
	NetlogonBinding.c \
	NetrMemory.c \
	NetrStubMemory.c \
	NetrCredentials.c \
	NetrServerReqChallenge.c \
	NetrServerAuthenticate.c \
	NetrServerAuthenticate2.c \
	NetrServerAuthenticate3.c \
	schannel.c \
	NetrSamLogon.c \
	NetrSamLogoff.c \
	NetrSamLogonEx.c \
	NetrEnumerateTrustedDomainsEx.c \
	DsrEnumerateDomainTrusts.c

libnetlogon_cs_la_SOURCES = netlogon_cstub.c
libnetlogon_ss_la_SOURCES = netlogon_sstub.c

LIBRPC_INCDIR = $(librpc_srcdir)/Include
LIBRPC_INCLUDES = -I$(LIBRPC_INCDIR)
INCLUDES = $(DCERPC_INCLUDES) $(UNISTR_INCLUDES) $(LIBRPC_INCLUDES)

FEATURETEST_FLAGS="-D_XOPEN_SOURCE=500 -D__EXTENSIONS__ -D_REENTRANT"

libnetlogon_cs_la_CPPFLAGS = $(FEATURETEST_FLAGS)
libnetlogon_cs_la_CFLAGS = $(FEATURETEST_FLAGS)

libnetlogon_la_CFLAGS = -g $(INCLUDES) -DLIBRPC_BUILD $(FEATURETEST_FLAGS) $(AM_CFLAGS)
libnetlogon_la_LDFLAGS = $(DCERPC_LDFLAGS) $(UNISTR_LDFLAGS)
libnetlogon_la_LIBADD = \
	-ldcerpc -lunistr -lsecdesc \
	$(top_builddir)/MsRpc/libmsrpc.la \
	$(top_builddir)/Alloc/liballoc.la \
        $(top_builddir)/Error/liberror.la \
	libnetlogon_cs.la

CLEANFILES = netlogon_h.h netlogon_cstub.c netlogon_sstub.c

BUILT_SOURCES = netlogon_cstub.c netlogon_h.h

netlogon_cstub.c netlogon_h.h: $(idl_srcdir)/netlogon.idl
	$(IDL) $(IDLFLAGS) -keep c_source -header netlogon_h.h -I$(idl_srcdir) $(LIBRPC_INCLUDES) $<
