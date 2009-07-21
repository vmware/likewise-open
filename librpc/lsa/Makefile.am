librpc_srcdir=$(top_srcdir)
libmsrpc_srcdir=$(top_srcdir)/msrpc
idl_srcdir=$(top_srcdir)/idl

noinst_LTLIBRARIES = liblsa_cs.la liblsa.la

liblsa_la_SOURCES =         \
    lsa_binding.c           \
    lsa_memory.c            \
    lsa_stubmemory.c        \
    lsa_openpolicy2.c       \
    lsa_close.c             \
    lsa_lookupnames.c       \
    lsa_lookupnames2.c      \
    lsa_lookupnames3.c      \
    lsa_lookupsids.c        \
    lsa_queryinfopolicy.c   \
    lsa_queryinfopolicy2.c

liblsa_cs_la_SOURCES = lsa_cstub.c

LIBRPC_INCDIR = $(librpc_srcdir)/include
LIBRPC_INCLUDES = -I$(LIBRPC_INCDIR)
INCLUDES = $(DCERPC_INCLUDES) $(UNISTR_INCLUDES) $(LIBRPC_INCLUDES)

FEATURETEST_FLAGS="-D_XOPEN_SOURCE=500 -D__EXTENSIONS__ -D_REENTRANT"

liblsa_cs_la_CPPFLAGS = $(FEATURETEST_FLAGS)
liblsa_cs_la_CFLAGS = \
    -DDCERPC_STUB_BUILD \
    $(FEATURETEST_FLAGS)

liblsa_la_CFLAGS=-g $(INCLUDES) -DLIBRPC_BUILD $(FEATURETEST_FLAGS) $(AM_CFLAGS)
liblsa_la_LDFLAGS=$(DCERPC_LDFLAGS) $(UNISTR_LDFLAGS)
liblsa_la_LIBADD= \
    -ldcerpc -lunistr -llwbase \
    liblsa_cs.la

CLEANFILES = lsa_h.h lsa_cstub.c lsa_sstub.c

BUILT_SOURCES = lsa_h.h

lsa_cstub.c lsa_h.h: $(idl_srcdir)/lsa.idl lsa.acf
	$(IDL) $(IDLFLAGS) -keep c_source -header lsa_h.h -I$(idl_srcdir) \
		$(LIBRPC_INCLUDES) $<
