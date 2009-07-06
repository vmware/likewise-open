librpc_srcdir=$(top_srcdir)
idl_srcdir=$(top_srcdir)/Idl
error_srcdir=$(top_srcdir)/Error

lib_LTLIBRARIES = libnetapi.la

libnetapi_la_SOURCES = \
	globals.c \
	NetConnection.c \
	NetMemory.c \
        NetUserEnum.c \
	NetUserAdd.c \
	NetLibUserInfo.c \
	NetUserDel.c \
	NetUser.c \
	NetUserGetInfo.c \
	NetUserSetInfo.c \
	GroupInfo.c \
	NetUserGetLocalGroups.c \
	NetLocalGroupEnum.c \
	NetLocalGroupAdd.c \
	NetLocalGroupDel.c \
	NetLocalGroupGetInfo.c \
	NetLocalGroupSetInfo.c \
	NetLocalGroupMembers.c \
	NetLocalGroupGetMembers.c \
	NetJoinDomain.c \
	JoinLocal.c \
	NetUnjoinDomain.c \
	UnjoinLocal.c \
	NetApiBufferFree.c \
	NetUserChangePassword.c \
	NetGetDomainName.c \
	NetMachineChangePassword.c \
	LdapUtil.c \
	NetDebug.c \
	MachinePassword.c \
	NetGetDcName.c \
	NetHostInfo.c

libnetapi_la_CPPFLAGS = $(AM_CPPFLAGS) @LWBASE_INCLUDES@

libnetapi_la_LIBADD = \
	-lunistr -lldap_r -lktkrb5 -llwioclient -lcrypto \
	$(top_builddir)/Samr/libsamr.la \
	$(top_builddir)/lsa/liblsa.la \
	$(top_builddir)/Netlogon/libnetlogon.la \
	$(top_builddir)/MsRpc/libmsrpc.la \
	$(top_builddir)/Mpr/libmpr.la \
	@LWBASE_LIBS@

if HAVE_LIBTDB
libnetapi_la_LIBADD += -ltdb
endif

DCERPC_INCLUDES = $(dcerpc_srcdir)/include
LIBRPC_INCLUDES = $(librpc_srcdir)/Include
UNISTR_INCDIR = $(libunistr_srcdir)/include
LIBTDB_INCDIR = $(libtdb_srcdir)/include
INCLUDES = -I$(DCERPC_INCLUDES) -I$(LIBRPC_INCLUDES) -I$(UNISTR_INCDIR) \
	-I$(LIBTDB_INCDIR) -I$(error_srcdir) -I$(librpc_srcdir) \
        -I$(PSTORE_INCLUDES) -I$(KEYTAB_INCLUDES)

FEATURETEST_FLAGS="-D__EXTENSIONS__ -D_REENTRANT"

libnetapi_la_CFLAGS = $(AM_CFLAGS) $(INCLUDES) -DLIBRPC_BUILD $(FEATURETEST_FLAGS)
libnetapi_la_LDFLAGS = \
	$(AM_LDLAGS) $(DCERPC_LDFLAGS) $(PSTORE_LDFLAGS) $(KEYTAB_LDFLAGS)

