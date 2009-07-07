librpc_srcdir=$(top_srcdir)
libsamr_builddir=$(top_builddir)/Samr
liballoc_builddir=$(top_builddir)/alloc
libmsrpc_builddir=$(top_builddir)/MsRpc
liblsa_builddir=$(top_builddir)/lsa
libnetlogon_builddir=$(top_builddir)/netlogon
libmpr_builddir=$(top_builddir)/Mpr
libnetapi_builddir=$(top_builddir)/NetLib
libdssetup_builddir=$(top_builddir)/dssetup

bin_PROGRAMS = testrpc

testrpc_SOURCES = \
	TestRpc.c \
	Params.c \
	Lsa.c \
	Samr.c \
	Netlogon.c \
	NetApi.c \
        dssetup.c \
	Mpr.c

LIBRPC_INCDIR = $(librpc_srcdir)/Include
INCLUDES = -I$(LIBRPC_INCDIR) -I$(librpc_srcdir)

testrpc_CPPFLAGS = $(AM_CPPFLAGS) @LWBASE_INCLUDES@
testrpc_CFLAGS = -g $(INCLUDES) -D_GNU_SOURCE -DLIBRPC_BUILD $(AM_CFLAGS)
testrpc_LDFLAGS = $(PSTORE_LDFLAGS) $(KEYTAB_LDFLAGS) -rpath ${libdir}
testrpc_LDADD = \
	$(liballoc_builddir)/liballoc.la \
	$(libnetapi_builddir)/libnetapi.la \
	@LWBASE_LIBS@
