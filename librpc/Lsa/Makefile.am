librpc_srcdir=$(top_srcdir)
libmsrpc_srcdir=$(top_srcdir)/MsRpc
idl_srcdir=$(top_srcdir)/Idl

lib_LTLIBRARIES = liblsa_cs.la liblsa_ss.la liblsa.la

liblsa_la_SOURCES = \
	globals.c \
	LsaBinding.c \
	LsaMemory.c \
	LsaStubMemory.c \
	LsaOpenPolicy2.c \
	LsaClose.c \
	LsaLookupNames.c \
	LsaLookupNames2.c \
	LsaLookupSids.c \
	LsaQueryInfoPolicy.c \
	LsaQueryInfoPolicy2.c
	globals.c LsaBinding.c LsaMemory.c LsaStubMemory.c \
	LsaOpenPolicy2.c LsaClose.c LsaLookupNames.c LsaLookupNames2.c \
	LsaLookupSids.c LsaQueryInfoPolicy.c LsaQueryInfoPolicy2.c

liblsa_cs_la_SOURCES = lsa_cstub.c
liblsa_ss_la_SOURCES = lsa_sstub.c

LIBRPC_INCDIR = $(librpc_srcdir)/Include
LIBRPC_INCLUDES = -I$(LIBRPC_INCDIR)
INCLUDES = $(DCERPC_INCLUDES) $(UNISTR_INCLUDES) $(LIBRPC_INCLUDES)

liblsa_la_CFLAGS=-g $(INCLUDES) -DLIBRPC_BUILD $(AM_CFLAGS)
liblsa_la_LDFLAGS=$(DCERPC_LDFLAGS) $(UNISTR_LDFLAGS)
liblsa_la_LIBADD= \
	-ldcerpc -lunistr -lsecdesc \
	$(top_builddir)/MsRpc/libmsrpc.la \
	liblsa_cs.la

CLEANFILES = lsa_h.h lsa_cstub.c lsa_sstub.c

BUILT_SOURCES = lsa_cstub.c lsa_h.h

lsa_cstub.c lsa_h.h: $(idl_srcdir)/lsa.idl
	$(IDL) $(IDLFLAGS) -keep c_source -header lsa_h.h -I$(idl_srcdir) \
		$(LIBRPC_INCLUDES) $<
