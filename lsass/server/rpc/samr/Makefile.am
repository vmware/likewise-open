srv_srcdir = $(top_srcdir)/server
rpc_srcdir = $(srv_srcdir)/rpc
idl_srcdir = $(rpc_srcdir)/idl

lib_LTLIBRARIES = libsamr_ss.la libsamr_srv.la

libsamr_srv_la_SOURCES = \
    globals.c \
    samr.c \
    samr_srv.c

LSASS_SRV_INCLUDES = -I$(srv_srcdir)/include
LSASS_INCLUDES = -I$(top_srcdir)/include
IDL_PASS_INCLUDES = \
    $(CPPFLAGS) \
    @NETAPI_INCLUDES@ \
    @LIBSECDESC_INCLUDES@

libsamr_srv_la_CFLAGS = \
    -DLIBRPC_BUILD \
    $(LSASS_SRV_INCLUDES) \
    $(LSASS_INCLUDES) \
    @DCERPC_INCLUDES@ \
    @LIBUNISTR_INCLUDES@ \
    @LIBSECDESC_INCLUDES@

libsamr_srv_la_LIBADD = \
    @DCERPC_LIBS@ \
    @LIBUNISTR_LIBS@ \
    @LIBSECDESC_LIBS@ \
    libsamr_ss.la

libsamr_ss_la_SOURCES = samr_sstub.c

libsamr_ss_la_CFLAGS = \
    @DCERPC_INCLUDES@ \
    @LIBUNISTR_INCLUDES@ \
    @LIBSECDESC_INCLUDES@

CLEAN_FILES = samr_cstub.c samr_sstub.c samr_h.h

BUILT_SOURCES = samr_cstub.c samr_sstub.c samr_h.h

samr_sstub.c samr_cstub.c samr_h.h: $(idl_srcdir)/samr.idl
	$(IDL) $(IDLFLAGS) -keep c_source -header samr_h.h -I$(idl_srcdir) \
                $(IDL_PASS_INCLUDES) $<
