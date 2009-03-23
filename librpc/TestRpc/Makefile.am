librpc_srcdir=$(top_srcdir)
libsamr_builddir=$(top_srcdir)/Samr
liballoc_builddir=$(top_srcdir)/Alloc
libmsrpc_builddir=$(top_srcdir)/MsRpc
liblsa_builddir=$(top_srcdir)/Lsa
libnetlogon_builddir=$(top_srcdir)/Netlogon
libmpr_builddir=$(top_srcdir)/Mpr
libnetapi_builddir=$(top_srcdir)/NetLib

bin_PROGRAMS = testrpc

testrpc_SOURCES = \
	TestRpc.c \
	Params.c \
	Lsa.c \
	Samr.c \
	Netlogon.c \
	NetApi.c \
	Mpr.c

LIBRPC_INCDIR = $(librpc_srcdir)/Include
INCLUDES = -I$(LIBRPC_INCDIR) -I$(librpc_srcdir)

testrpc_CPPFLAGS = $(AM_CPPFLAGS) @LWBASE_INCLUDES@
testrpc_CFLAGS = -g $(INCLUDES) -D_GNU_SOURCE -DLIBRPC_BUILD $(AM_CFLAGS)
testrpc_LDFLAGS = $(PSTORE_LDFLAGS) $(KEYTAB_LDFLAGS) -rpath ${libdir}
testrpc_LDADD = \
	$(libsamr_builddir)/libsamr.la \
	$(liblsa_builddir)/liblsa.la \
	$(libnetlogon_builddir)/libnetlogon.la \
	$(liballoc_builddir)/liballoc.la \
	$(libmpr_builddir)/libmpr.la \
	$(libmsrpc_builddir)/libmsrpc.la \
	$(libnetapi_builddir)/libnetapi.la \
	@LWBASE_LIBS@
