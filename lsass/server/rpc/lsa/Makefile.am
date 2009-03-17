srv_srcdir = $(top_srcdir)/server
rpc_srcdir = $(srv_srcdir)/rpc
idl_srcdir = $(rpc_srcdir)/idl

lib_LTLIBRARIES = liblsa_ss.la liblsa_srv.la

liblsa_srv_la_SOURCES = \
    globals.c \
    lsa.c \
    lsa_srv.c \
    lsa_openpolicy2.c \
    lsa_close.c \
    lsa_lookupnames.c \
    lsa_lookupsids.c \
    lsa_lookupnames2.c \
    lsa_queryinfopolicy.c \
    lsa_queryinfopolicy2.c \
    lsa_contexthandle.c

LSASS_SRV_INCLUDES = -I$(srv_srcdir)/include
LSASS_INCLUDES = -I$(top_srcdir)/include
RPCCTL_INCLUDES = -I$(rpc_srcdir)/rpcctl
DCEIDL_PASS_INCLUDES = \
    $(CPPFLAGS) \
    @NETAPI_INCLUDES@ \
    @LIBSECDESC_INCLUDES@

FEATURETEST_FLAGS="-D_XOPEN_SOURCE=500 -D__EXTENSIONS__ -D_REENTRANT"

liblsa_srv_la_CFLAGS = \
    -DLIBRPC_BUILD \
    $(FEATURETEST_FLAGS) \
    $(LSASS_SRV_INCLUDES) \
    $(LSASS_INCLUDES) \
    $(LIBRPC_INCLUDES) \
    $(RPCCTL_INCLUDES) \
    @DCERPC_INCLUDES@ \
    @LIBUNISTR_INCLUDES@ \
    @LIBSECDESC_INCLUDES@

liblsa_srv_la_LIBADD = \
    @DCERPC_LIBS@ \
    @LIBUNISTR_LIBS@ \
    @LIBSECDESC_LIBS@ \
    $(rpc_srcdir)/rpcctl/librpcctl.la \
    liblsa_ss.la

liblsa_ss_la_SOURCES = lsa_sstub.c

liblsa_ss_la_CFLAGS = \
    -DDCERPC_STUB_BUILD \
    $(FEATURETEST_FLAGS) \
    @DCERPC_INCLUDES@ \
    @LIBUNISTR_INCLUDES@ \
    @LIBSECDESC_INCLUDES@

CLEAN_FILES = lsa_cstub.c lsa_sstub.c lsa_h.h

BUILT_SOURCES = lsa_cstub.c lsa_sstub.c lsa_h.h

lsa_sstub.c lsa_cstub.c lsa_h.h: $(idl_srcdir)/lsa.idl
	$(IDL) $(IDLFLAGS) -keep c_source -header lsa_h.h -I$(idl_srcdir) \
		$(DCEIDL_PASS_INCLUDES) $<
