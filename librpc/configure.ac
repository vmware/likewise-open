AC_INIT(Idl/samr.idl)
AM_INIT_AUTOMAKE(librpc,0.1)
AC_CONFIG_HEADER([Include/config.h])

# Checks for programs.
AC_PROG_CC
AC_PROG_LIBTOOL

DCERPC_INCLUDES=""
DCERPC_LDFLAGS=""
DCERPC_PATH=""

PSTORE_INCLUDES=""
PSTORE_LDFLAGS=""

AM_CFLAGS="$AM_CFLAGS -Wall -fno-strict-aliasing -Werror"

AC_SUBST(LTLDFLAGS)
AC_SUBST(AM_CFLAGS)

# Handle options
AC_ARG_WITH(
	dcerpc-dir, 
	[  --with-dcerpc-dir=<prefix>      look for dcerpc headers/libraries in <prefix>],
	[
	    DCERPC_INCLUDES=-I$withval/include
	    DCERPC_LDFLAGS=-L$withval/lib
	    DCERPC_PATH=$withval/bin
	]
)

AC_ARG_WITH(
	dcerpc-libdir, 
	[  --with-dcerpc-libdir=<libdir>   look for dcerpc libraries in <libdir>],
	[
	    DCERPC_LDFLAGS="-L$withval"
	]
)

AC_ARG_WITH(
	unistr-dir, 
	[  --with-unistr-dir=<prefix>      look for dcerpc headers/libraries in <prefix>],
	[
	    UNISTR_INCLUDES=-I$withval/include
	    UNISTR_LDFLAGS=-L$withval/lib
	]
)

AC_ARG_WITH(
	unistr-libdir, 
	[  --with-unistr-libdir=<libdir>   look for dcerpc libraries in <libdir>],
	[
	    UNISTR_LDFLAGS="-L$withval"
	]
)

AC_ARG_WITH(
	ldap-dir, 
	[  --with-ldap-dir=<prefix>        look for ldap headers/libraries in <prefix>],
	[
	    LDAP_INCLUDES=-I$withval/include
	    LDAP_LDFLAGS=-L$withval/lib
	]
)

AC_ARG_WITH(
	ldap-libdir, 
	[  --with-ldap-libdir=<libdir>     look for ldap libraries in <libdir>],
	[
	    LDAP_LDFLAGS="-L$withval"
	]
)

AC_ARG_WITH(
	pstore-dir, 
	[  --with-pstore-dir=<prefix>      look for pstore headers/libraries in <prefix>],
	[
	    PSTORE_INCLUDES=-I$withval/include
	    PSTORE_LDFLAGS="-L$withval/lib"
	]
)

AC_ARG_WITH(
	pstore-libdir,
	[  --with-pstore-libdir=<prefix>   look for pstore libraries in <libdir>],
	[
	    PSTORE_LDFLAGS="-L$withval"
	]
)

AC_ARG_WITH(
	lwnet-dir, 
	[  --with-lwnet-dir=<prefix>       look for pstore headers/libraries in <prefix>],
	[
	    LWNET_INCLUDES=-I$withval/include
	    LWNET_LDFLAGS="-L$withval/lib"
	]
)

AC_ARG_WITH(
	lwnet-libdir,
	[  --with-lwnet-libdir=<prefix>    look for pstore libraries in <libdir>],
	[
	    LWNET_LDFLAGS="-L$withval"
	]
)

AC_ARG_WITH(
	keytab-dir, 
	[  --with-keytab-dir=<prefix>      look for keytab headers/libraries in <prefix>],
	[
	    KEYTAB_INCLUDES="-I$withval/include"
	    KEYTAB_LDFLAGS="-L$withval/lib"
	]
)

AC_ARG_WITH(
	keytab-libdir,
	[  --with-keytab-libdir=<prefix>   look for keytab libraries in <libdir>],
	[
	    KEYTAB_LDFLAGS="-L$withval"
	]
)

AC_ARG_WITH(
	keytab-dir,
	[  --with-secdesc-dir=<prefix>     look for secdesc headers/libraries in <prefix>],
	[
	    SECDESC_INCLUDES="-I$withval/include"
	    SECDESC_LDFLAGS="-L$withval/lib"
	]
)

AC_ARG_WITH(
	secdesc-libdir,
	[  --with-secdesc-libdir=<prefix>  look for keytab libraries in <libdir>],
	[
	    SECDESC_LDFLAGS="-L$withval"	]
)

# lwbase

AC_ARG_WITH([lwbase],
        [AC_HELP_STRING([--with-lwbase=<dir>], [use lwbase located in prefix <dir>])],
        [
		LWBASE_INCLUDES="-I$withval/include"
        LWBASE_LDFLAGS="-L$withval/lib"
        ])

AC_ARG_WITH([lwbase-includes],
        [AC_HELP_STRING([--with-lwbase-includes=<dir>], [use lwbase includes located in <dir>])],
        [
		LWBASE_INCLUDES="-I$withval"
        ])

AC_ARG_WITH([lwbase-libs],
        [AC_HELP_STRING([--with-lwbase-libs=<dir>], [use lwbase libs located in <dir>])],
        [
		LWBASE_LDFLAGS="-L$withval"
        ])

AC_CHECK_HEADER([lw/base.h],,[AC_MSG_ERROR([Could not find lwbase headers])])
AC_CHECK_LIB([lwbase_nothr], [LwRtlMemoryAllocate], [LWBASE_NOTHR_LIBS="-llwbase_nothr"], [AC_MSG_ERROR([Could not find liblwbase_nothr])], [$LWBASE_LDFLAGS])
AC_CHECK_LIB([lwbase], [LwInterlockedIncrement], [LWBASE_LIBS="-llwbase"], [AC_MSG_ERROR([Could not find liblwbase])], [$LWBASE_LDFLAGS])

AC_SUBST(LWBASE_INCLUDES)
AC_SUBST(LWBASE_LDFLAGS)
AC_SUBST(LWBASE_NOTHR_LIBS)
AC_SUBST(LWBASE_LIBS)

AC_SUBST(DCERPC_INCLUDES)
AC_SUBST(DCERPC_LDFLAGS)
AC_SUBST(DCERPC_PATH)
AC_SUBST(UNISTR_INCLUDES)
AC_SUBST(UNISTR_LDFLAGS)
AC_SUBST(LDAP_INCLUDES)
AC_SUBST(LDAP_LDFLAGS)
AC_SUBST(PSTORE_INCLUDES)
AC_SUBST(PSTORE_LDFLAGS)
AC_SUBST(LWNET_INCLUDES)
AC_SUBST(LWNET_LDFLAGS)
AC_SUBST(KEYTAB_INCLUDES)
AC_SUBST(KEYTAB_LDFLAGS)
AC_SUBST(SECDESC_INCLUDES)
AC_SUBST(SECDESC_LDFLAGS)

for i in $CPPFLAGS ${DCERPC_INCLUDES} ${UNISTR_INCLUDES}; do
    j=`echo $i | grep '^-I'`
    if test x != x"$j"
    then
        IDLFLAGS="$IDLFLAGS $j"
    fi
done
AC_SUBST(IDLFLAGS)

# Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST
AC_TYPE_SIZE_T

AC_CHECK_SIZEOF([long int])

# Checks for library functions.
AC_CHECK_FUNCS(strndup)


LIBICONV_LDFLAGS=""
AC_CHECK_LIB(iconv, iconv, [LIBICONV_LDFLAGS="-liconv"])

# Checks for libraries.
CPPFLAGS="$CPPFLAGS $PSTORE_INCLUDES" \
LDFLAGS="$LDFLAGS $PSTORE_LDFLAGS" \
AC_CHECK_LIB(lwpsapi, LwpsOpenPasswordStore, [],
     AC_MSG_ERROR([PStore library not found]), [$LIBICONV_LDFLAGS])

CPPFLAGS="$CPPFLAGS $DCERPC_INCLUDES" \
LDFLAGS="$LDFLAGS $DCERPC_LDFLAGS" \
AC_CHECK_LIB(dcerpc, rpc__init, [],
     AC_MSG_ERROR([DCERPC library not found]))

CPPFLAGS="$CPPFLAGS $LWNET_INCLUDES" \
LDFLAGS="$LDFLAGS $LWNET_LDFLAGS" \
AC_CHECK_LIB(lwnetutils, LWNetFreeMemory, [],
     AC_MSG_ERROR([Netlogon daemon utils library not found]))

CPPFLAGS="$CPPFLAGS $LWNET_INCLUDES" \
LDFLAGS="$LDFLAGS $LWNET_LDFLAGS" \
AC_CHECK_LIB(lwnetclientapi, LWNetGetDCName, [],
     AC_MSG_ERROR([Netlogon daemon client library not found]))

AC_CHECK_LIB(tdb, tdb_fetch, [
		AC_DEFINE(HAVE_LIBTDB, 1, [Set 1 one if the tdbbackend should be used])
		have_libtdb=true
		],
		[have_libtdb=false])
AM_CONDITIONAL([HAVE_LIBTDB], [test x$have_libtdb = xtrue])

AC_CHECK_LIB(lwioclient, LwIoGetThreadAccessToken, [], AC_MSG_ERROR([liblwioclient not found]))

# Checks for header files.
AC_HEADER_STDC
AC_HEADER_STDBOOL
AC_CHECK_HEADERS([stdlib.h string.h strings.h inttypes.h sys/varargs.h])

# Check for programs
AC_PATH_PROG([IDL], [dceidl], [no], [$PATH:$DCERPC_PATH])

if test x"$IDL" = x"no"; then
   AC_MSG_ERROR([DCERPC IDL compiler not found])
fi

# Check for moonunit bits
AC_CHECK_HEADERS([moonunit/moonunit.h], [HAVE_MOONUNIT_H=true], [HAVE_MOONUNIT_H=false])
AC_CHECK_LIB(moonunit, Mu_Interface_Result, [HAVE_MOONUNIT_LIB=true], [HAVE_MOONUNIT_LIB=false])
AC_CHECK_PROG(MOONUNIT_STUB, moonunit-stub, moonunit-stub, false)

if test "${MOONUNIT_STUB}" = "moonunit-stub" && $HAVE_MOONUNIT_H && $HAVE_MOONUNIT_LIB
then
	HAVE_MOONUNIT="true"
	LIB_moonunit="-lmoonunit"
	AC_DEFINE([HAVE_MOONUNIT], [], [Have MoonUnit])
else
	HAVE_MOONUNIT="false"
fi

AC_SUBST(LIB_moonunit)
AM_CONDITIONAL(HAVE_MOONUNIT, $HAVE_MOONUNIT)

AC_OUTPUT(Makefile
          Include/Makefile
          Error/Makefile
          alloc/Makefile
          MsRpc/Makefile
          Samr/Makefile
          lsa/Makefile
	  Krb5Pac/Makefile
          NetLib/Makefile
          Mpr/Makefile
          Netlogon/Makefile
          dssetup/Makefile
	  TestRpc/Makefile)
