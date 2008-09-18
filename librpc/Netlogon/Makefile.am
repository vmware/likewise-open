librpc_srcdir=$(top_srcdir)
idl_srcdir=$(top_srcdir)/Idl

lib_LTLIBRARIES = libnetlogon.la

libnetlogon_la_SOURCES = \
	netlogon_cstub.c NetlogonBinding.c NetrMemory.c globals.c NetrStubMemory.c \
	NetrCredentials.c NetrServerReqChallenge.c NetrServerAuthenticate2.c \
	NetrEnumerateTrustedDomainsEx.c DsrEnumerateDomainTrusts.c

LIBRPC_INCDIR=$(librpc_srcdir)/Include
LIBRPC_INCLUDES=-I$(LIBRPC_INCDIR)
INCLUDES=$(DCERPC_INCLUDES) $(UNISTR_INCLUDES) $(LIBRPC_INCLUDES)

libnetlogon_la_CFLAGS=-g $(INCLUDES) -DLIBRPC_BUILD
libnetlogon_la_LDFLAGS=$(DCERPC_LDFLAGS) $(UNISTR_LDFLAGS)
libnetlogon_la_LIBADD=-ldcerpc -lunistr $(top_builddir)/MsRpc/libmsrpc.la \
	$(top_builddir)/Alloc/liballoc.la \
        $(top_builddir)/Error/liberror.la

CLEANFILES = netlogon_h.h netlogon_cstub.c netlogon_sstub.c

BUILT_SOURCES = netlogon_cstub.c netlogon_h.h

netlogon_cstub.c netlogon_h.h: $(idl_srcdir)/netlogon.idl
	$(IDL) $(IDLFLAGS) -keep c_source -header netlogon_h.h -I$(idl_srcdir) $(LIBRPC_INCLUDES) $<
