librpc_srcdir=$(top_srcdir)
libmsrpc_srcdir=$(top_srcdir)/MsRpc
idl_srcdir=$(top_srcdir)/idl

noinst_LTLIBRARIES = libsamr_cs.la libsamr.la

libsamr_la_SOURCES =             \
    globals.c                    \
    samr_binding.c               \
    samr_memory.c                \
    samr_stubmemory.c            \
    samr_close.c                 \
    samr_connect2.c              \
    samr_enumdomains.c           \
    samr_enumdomainusers.c       \
    samr_lookupdomain.c          \
    samr_lookupnames.c           \
    samr_opendomain.c            \
    samr_openuser.c              \
    samr_queryuserinfo.c         \
    samr_setuserinfo.c           \
    samr_setuserinfo2.c          \
    samr_createuser.c            \
    samr_deleteuser.c            \
    samr_createdomalias.c        \
    samr_deletedomalias.c        \
    samr_getaliasmembership.c    \
    samr_createdomgroup.c        \
    samr_deletedomgroup.c        \
    samr_opengroup.c             \
    samr_lookuprids.c            \
    samr_queryaliasinfo.c        \
    samr_setaliasinfo.c          \
    samr_addaliasmember.c        \
    samr_deletealiasmember.c     \
    samr_getmembersinalias.c     \
    samr_querydomaininfo.c       \
    samr_enumdomainaliases.c     \
    samr_openalias.c             \
    samr_connect3.c              \
    samr_connect4.c              \
    samr_connect5.c              \
    samr_createuser2.c           \
    samr_getuserpwinfo.c         \
    samr_changepassworduser2.c   \
    samr_getusergroups.c         \
    samr_querydisplayinfo.c

libsamr_cs_la_SOURCES = samr_cstub.c

LIBRPC_INCDIR = $(librpc_srcdir)/include
LIBRPC_INCLUDES = -I$(LIBRPC_INCDIR)
INCLUDES = $(DCERPC_INCLUDES) $(UNISTR_INCLUDES) $(LIBRPC_INCLUDES)

FEATURETEST_FLAGS="-D_XOPEN_SOURCE=500 -D__EXTENSIONS__ -D_REENTRANT"

libsamr_cs_la_CPPFLAGS = $(FEATURETEST_FLAGS)
libsamr_cs_la_CFLAGS = \
    -DDCERPC_STUB_BUILD \
    $(FEATURETEST_FLAGS)

libsamr_la_CFLAGS = -g $(INCLUDES) -DLIBRPC_BUILD $(FEATURETEST_FLAGS) $(AM_CFLAGS)
libsamr_la_LDFLAGS = $(DCERPC_LDFLAGS) $(UNISTR_LDFLAGS)
libsamr_la_LIBADD = \
    -ldcerpc -lunistr -llwbase \
    libsamr_cs.la

CLEANFILES = samr_h.h samr_cstub.c samr_sstub.c

BUILT_SOURCES = samr_h.h

samr_h.h samr_cstub.c: $(idl_srcdir)/samr.idl samr.acf
	$(IDL) $(IDLFLAGS) -keep c_source -header samr_h.h -I$(idl_srcdir) \
		$(LIBRPC_INCLUDES) $<
