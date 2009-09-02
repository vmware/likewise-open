librpc_srcdir=$(top_srcdir)

lib_LTLIBRARIES = libnetapi.la

libnetapi_la_SOURCES = \
	globals.c \
	net_connection.c              \
	net_memory.c                  \
        net_userenum.c                \
	net_useradd.c                 \
	net_userinfo.c                \
	net_userdel.c                 \
	net_user.c                    \
	net_usergetinfo.c             \
	net_usersetinfo.c             \
	net_groupinfo.c               \
	net_usergetlocalgroups.c      \
	net_localgroupenum.c          \
	net_localgroupadd.c           \
	net_localgroupdel.c           \
	net_localgroupgetinfo.c       \
	net_localgroupsetinfo.c       \
	net_localgroupmembers.c       \
	net_localgroupgetmembers.c    \
	net_joindomain.c              \
	joinlocal.c                   \
	net_unjoindomain.c            \
	unjoinlocal.c                 \
	net_bufferfree.c              \
	net_userchangepassword.c      \
	net_getdomainname.c           \
	net_machinechangepassword.c   \
	ldaputil.c                    \
	net_debug.c                   \
	machinepassword.c             \
	net_getdcname.c               \
	net_hostinfo.c

libnetapi_la_CPPFLAGS = $(AM_CPPFLAGS) @LWBASE_INCLUDES@

libnetapi_la_LIBADD = \
	-lunistr -lldap_r -lktkrb5 -llwioclient -lcrypto \
	$(LWBASE_LIBS) \
        $(LWADVAPI_LIBS) \
	$(top_builddir)/alloc/liballoc.la \
	$(top_builddir)/samr/libsamr.la \
	$(top_builddir)/lsa/liblsa.la \
	$(top_builddir)/netlogon/libnetlogon.la \
	$(top_builddir)/msrpc/libmsrpc.la \
	$(top_builddir)/krb5pac/libkrb5pac.la \
	$(top_builddir)/dssetup/libdssetup.la

if HAVE_LIBTDB
libnetapi_la_LIBADD += -ltdb
endif

LIBRPC_INCLUDES = -I$(librpc_srcdir)/include
LIBTDB_INCLUDES = -I$(libtdb_srcdir)/include
INCLUDES =                 \
    $(DCERPC_INCLUDES)     \
    $(LIBRPC_INCLUDES)     \
    $(UNISTR_INCLUDES)     \
    $(LIBTDB_INCDIR)       \
    $(PSTORE_INCLUDES)     \
    $(KEYTAB_INCLUDES)

FEATURETEST_FLAGS="-D__EXTENSIONS__ -D_REENTRANT"

libnetapi_la_CFLAGS =      \
    $(AM_CFLAGS)           \
    -DLIBRPC_BUILD         \
    $(INCLUDES)            \
    $(FEATURETEST_FLAGS)

libnetapi_la_LDFLAGS =     \
    $(AM_LDLAGS)           \
    $(LWBASE_LDFLAGS)      \
    $(LWADVAPI_LDFLAGS)    \
    $(DCERPC_LDFLAGS)      \
    $(PSTORE_LDFLAGS)      \
    $(KEYTAB_LDFLAGS)

