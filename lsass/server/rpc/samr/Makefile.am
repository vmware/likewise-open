srv_srcdir = $(top_srcdir)/server
rpc_srcdir = $(srv_srcdir)/rpc
idl_srcdir = $(rpc_srcdir)/idl

if ENABLE_STATIC_RPCSERVERS
noinst_LTLIBRARIES = libsamr_srv.la
else
rpcserver_LTLIBRARIES = libsamr_srv.la
noinst_LTLIBRARIES = libsamr_ss.la libsamr.la
endif

libsamr_la_SOURCES = \
    globals.c                    \
    samr.c                       \
    samr_cfg.c                   \
    samr_srv.c                   \
    samr_memory.c                \
    samr_connect.c               \
    samr_connect2.c              \
    samr_connect3.c              \
    samr_connect4.c              \
    samr_connect5.c              \
    samr_lookupdomain.c          \
    samr_enumdomains.c           \
    samr_opendomain.c            \
    samr_close.c                 \
    samr_lookupnames.c           \
    samr_openaccount.c           \
    samr_openuser.c              \
    samr_openalias.c             \
    samr_enumdomainaccounts.c    \
    samr_enumdomainusers.c       \
    samr_enumdomainaliases.c     \
    samr_queryuserinfo.c         \
    samr_queryaliasinfo.c        \
    samr_createaccount.c         \
    samr_createuser.c            \
    samr_createuser2.c           \
    samr_createdomalias.c        \
    samr_querydisplayinfo.c      \
    samr_querydomaininfo.c       \
    samr_getmembersinalias.c     \
    samr_getaliasmembership.c    \
    samr_contexthandle.c

DCEIDL_PASS_INCLUDES = \
    $(CPPFLAGS) \
    $(NETAPI_INCLUDES) \
    $(LWBASE_INCLUDES)

FEATURETEST_FLAGS="-D_XOPEN_SOURCE=500 -D__EXTENSIONS__ -D_REENTRANT"

libsamr_la_CFLAGS = \
    -DLIBRPC_BUILD          \
    $(AM_CFLAGS)            \
    $(FEATURETEST_FLAGS)    \
    -I$(srv_srcdir)/include \
    -I$(top_srcdir)/include \
    -I$(srv_srcdir)/api     \
    $(DCERPC_INCLUDES)      \
    $(LIBUNISTR_INCLUDES)   \
    $(LWBASE_INCLUDES)

libsamr_la_LIBADD = \
    $(DCERPC_LIBS)                        \
    $(NETAPI_LIBS)                        \
    $(LIBUNISTR_LIBS)                     \
    $(LWBASE_LIBS)                        \
    $(srv_srcdir)/api/liblsaserverapi.la  \
    $(srv_srcdir)/store/dsapi/libdsapi.la


libsamr_ss_la_SOURCES = \
    samr_sstub.c

libsamr_ss_la_CFLAGS = \
    -DDCERPC_STUB_BUILD   \
    $(FEATURETEST_FLAGS)  \
    $(DCERPC_INCLUDES)    \
    $(LIBUNISTR_INCLUDES) \
    $(LWBASE_INCLUDES)

libsamr_ss_la_LIBADD = \
    libsamr.la


libsamr_srv_la_SOURCES =
libsamr_srv_la_CFLAGS =
libsamr_srv_la_LIBADD = \
    libsamr_ss.la


CLEAN_FILES = samr_cstub.c samr_sstub.c samr_h.h

BUILT_SOURCES = samr_cstub.c samr_sstub.c samr_h.h

samr_sstub.c samr_cstub.c samr_h.h: $(idl_srcdir)/samr.idl samr.acf
	$(IDL) $(IDLFLAGS) -keep c_source -header samr_h.h -I$(idl_srcdir) \
                $(DCEIDL_PASS_INCLUDES) $<
