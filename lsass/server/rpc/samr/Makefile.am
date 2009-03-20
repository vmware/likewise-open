srv_srcdir = $(top_srcdir)/server
rpc_srcdir = $(srv_srcdir)/rpc
idl_srcdir = $(rpc_srcdir)/idl

lib_LTLIBRARIES = libsamr_ss.la libsamr_srv.la

libsamr_srv_la_SOURCES = \
    globals.c \
    samr.c \
    samr_srv.c \
    samr_memory.c \
    samr_connect.c \
    samr_connect2.c \
    samr_lookupdomain.c \
    samr_enumdomains.c \
    samr_opendomain.c \
    samr_close.c \
    samr_contexthandle.c

DCEIDL_PASS_INCLUDES = \
    $(CPPFLAGS) \
    @NETAPI_INCLUDES@ \
    @LIBSECDESC_INCLUDES@

FEATURETEST_FLAGS="-D_XOPEN_SOURCE=500 -D__EXTENSIONS__ -D_REENTRANT"

libsamr_srv_la_CFLAGS = \
    -DLIBRPC_BUILD \
    $(FEATURETEST_FLAGS) \
    -I$(srv_srcdir)/include \
    -I$(top_srcdir)/include \
    -I$(rpc_srcdir)/rpcctl \
    @DCERPC_INCLUDES@ \
    @LIBUNISTR_INCLUDES@ \
    @LIBSECDESC_INCLUDES@

libsamr_srv_la_LIBADD = \
    @DCERPC_LIBS@ \
    @NETAPI_LIBS@ \
    @LIBUNISTR_LIBS@ \
    @LIBSECDESC_LIBS@ \
    $(rpc_srcdir)/rpcctl/librpcctl.la \
    $(srv_srcdir)/store/dsapi/libdsapi.la \
    libsamr_ss.la

libsamr_ss_la_SOURCES = samr_sstub.c

libsamr_ss_la_CFLAGS = \
    -DDCERPC_STUB_BUILD \
    $(FEATURETEST_FLAGS) \
    @DCERPC_INCLUDES@ \
    @LIBUNISTR_INCLUDES@ \
    @LIBSECDESC_INCLUDES@

CLEAN_FILES = samr_cstub.c samr_sstub.c samr_h.h

BUILT_SOURCES = samr_cstub.c samr_sstub.c samr_h.h

samr_sstub.c samr_cstub.c samr_h.h: $(idl_srcdir)/samr.idl
	$(IDL) $(IDLFLAGS) -keep c_source -header samr_h.h -I$(idl_srcdir) \
                $(DCEIDL_PASS_INCLUDES) $<
