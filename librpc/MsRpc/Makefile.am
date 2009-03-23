librpc_srcdir=$(top_srcdir)
idl_srcdir=$(top_srcdir)/Idl
LIBRPC_INCDIR = $(librpc_srcdir)/Include

lib_LTLIBRARIES = libmsrpc.la

libmsrpc_la_SOURCES = \
	UnicodeString.c \
	UnicodeStringEx.c \
	md4.c \
	md5.c \
	hmac_md5.c \
	rc4.c \
	des.c \
	smbcrypto.c \
	random.c \
	MemPtr.c \
	errcheck.c \
	LMcrypt.c \
	sidhelper.c

noinst_PROGRAMS=md4test md5test cryptotest randomtest destest

md4test_SOURCES=md4test.c
md4test_CFLAGS=-g
md4test_LDADD=libmsrpc.la

md5test_SOURCES=md5test.c
md5test_CFLAGS=-g
md5test_LDADD=libmsrpc.la

cryptotest_SOURCES=cryptotest.c
cryptotest_CFLAGS=-g
cryptotest_LDADD=libmsrpc.la

randomtest_SOURCES=randomtest.c
randomtest_CFLAGS=-g
randomtest_LDADD=libmsrpc.la

destest_SOURCES=destest.c
destest_CFLAGS=-g
destest_LDADD=libmsrpc.la

LIBRPC_INCLUDES = -I$(LIBRPC_INCDIR)
INCLUDES = $(DCERPC_INCLUDES) $(UNISTR_INCLUDES) $(LIBRPC_INCLUDES)

FEATURETEST_FLAGS="-D__EXTENSIONS__ -D_REENTRANT -D_BSD_SOURCE"

libmsrpc_la_CPPFLAGS = $(AM_CPPFLAGS) @LWBASE_INCLUDES@
libmsrpc_la_CFLAGS = -g $(INCLUDES) -DLIBRPC_BUILD $(FEATURETEST_FLAGS) $(AM_CFLAGS)
libmsrpc_la_LDFLAGS = $(DCERPC_LDFLAGS) $(UNISTR_LDFLAGS)
libmsrpc_la_LIBADD = -ldcerpc -lunistr @LWBASE_LIBS@
