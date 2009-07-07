AC_INIT(lwadvapi, 0.1, support@likewisesoftware.com)
AM_INIT_AUTOMAKE([-Wall -Werror foreign])
AC_CONFIG_HEADER([include/config.h])
AC_CANONICAL_HOST

AM_CFLAGS="$AM_CFLAGS -Wall -Werror -fno-strict-aliasing"

AC_SUBST(AM_CFLAGS)

AC_USE_SYSTEM_EXTENSIONS

# Checks for programs.
AC_PROG_CC
AC_PROG_LIBTOOL

# libunistr

AC_ARG_WITH([libunistr],
        [AC_HELP_STRING([--with-libunistr=<dir>], [use libunistr located in prefix <dir>])],
        [
		LIBUNISTR_INCLUDES="-I$withval/include"
                LIBUNISTR_LDFLAGS="-L$withval/lib"
        ])

AC_ARG_WITH([libunistr-includes],
        [AC_HELP_STRING([--with-libunistr-includes=<dir>], [use libunistr includes located in <dir>])],
        [
		LIBUNISTR_INCLUDES="-I$withval"
        ])

AC_ARG_WITH([libunistr-libs],
        [AC_HELP_STRING([--with-libunistr-libs=<dir>], [use libunistr libs located in <dir>])],
        [
		LIBUNISTR_LDFLAGS="-L$withval"
        ])

AC_SUBST(LIBUNISTR_INCLUDES)
AC_SUBST(LIBUNISTR_LDFLAGS)


# libuuid

AC_ARG_WITH([libuuid],
        [AC_HELP_STRING([--with-libuuid=<dir>], [use libuuid located in prefix <dir>])],
        [
                LIBUUID_INCLUDES="-I$withval/include"
                LIBUUID_LDFLAGS="-L$withval/lib"
        ])

AC_ARG_WITH([libuuid-includes],
        [AC_HELP_STRING([--with-libuuid-includes=<dir>], [use libuuid includes located in <dir>])],
        [
                LIBUUID_INCLUDES="-I$withval/include"
        ])

AC_ARG_WITH([libuuid-libs],
        [AC_HELP_STRING([--with-libuuid-libs=<dir>], [use libuuid libs located in <dir>])],
        [
                LIBUUID_LDFLAGS="-L$withval/lib"
        ])

AC_SUBST(LIBUUID_INCLUDES)
AC_SUBST(LIBUUID_LDFLAGS)

# krb5

AC_ARG_WITH([krb5],
        [AC_HELP_STRING([--with-krb5=<dir>], [use krb5 located in prefix <dir>])],
        [
		KRB5_INCLUDES="-I$withval/include"
                KRB5_LDFLAGS="-L$withval/lib"
        ])

AC_ARG_WITH([krb5-includes],
        [AC_HELP_STRING([--with-krb5-includes=<dir>], [use krb5 includes located in <dir>])],
        [
		KRB5_INCLUDES="-I$withval"
        ])

AC_ARG_WITH([krb5-libs],
        [AC_HELP_STRING([--with-krb5-libs=<dir>], [use krb5 libs located in <dir>])],
        [
		KRB5_LDFLAGS="-L$withval"
        ])

KRB5_LIBS="-lgssapi_krb5 -lkrb5 -lk5crypto"

AC_SUBST(KRB5_INCLUDES)
AC_SUBST(KRB5_LDFLAGS)
AC_SUBST(KRB5_LIBS)

# pstore

AC_ARG_WITH([pstore],
        [AC_HELP_STRING([--with-pstore=<dir>], [use pstore located in prefix <dir>])],
        [
		PSTORE_INCLUDES="-I$withval/include"
                PSTORE_LDFLAGS="-L$withval/lib"
        ])

AC_ARG_WITH([pstore-includes],
        [AC_HELP_STRING([--with-pstore-includes=<dir>], [use pstore includes located in <dir>])],
        [
		PSTORE_INCLUDES="-I$withval"
        ])

AC_ARG_WITH([pstore-libs],
        [AC_HELP_STRING([--with-pstore-libs=<dir>], [use pstore libs located in <dir>])],
        [
		PSTORE_LDFLAGS="-L$withval"
        ])

PSTORE_LIBS="-llwpsapi"

AC_SUBST(PSTORE_INCLUDES)
AC_SUBST(PSTORE_LDFLAGS)
AC_SUBST(PSTORE_LIBS)

# openldap

AC_ARG_WITH([openldap],
        [AC_HELP_STRING([--with-openldap=<dir>], [use openldap located in prefix <dir>])],
        [
		OPENLDAP_INCLUDES="-I$withval/include"
                OPENLDAP_LDFLAGS="-L$withval/lib"
        ])

AC_ARG_WITH([openldap-includes],
        [AC_HELP_STRING([--with-openldap-includes=<dir>], [use openldap includes located in <dir>])],
        [
		OPENLDAP_INCLUDES="-I$withval"
        ])

AC_ARG_WITH([openldap-libs],
        [AC_HELP_STRING([--with-openldap-libs=<dir>], [use openldap libs located in <dir>])],
        [
		OPENLDAP_LDFLAGS="-L$withval"
        ])

OPENLDAP_LIBS="-lldap_r -llber"

AC_SUBST(OPENLDAP_INCLUDES)
AC_SUBST(OPENLDAP_LDFLAGS)
AC_SUBST(OPENLDAP_LIBS)

AC_CHECK_TYPES([socklen_t], [], [], [AC_INCLUDES_DEFAULT
#include <sys/types.h>
#include <sys/socket.h>
])

AC_CHECK_SIZEOF(unsigned long)
AC_CHECK_SIZEOF(long)

AC_MSG_CHECKING([if getsockname takes socklen_t*])
AC_TRY_COMPILE([
#include <sys/types.h> 
#include <sys/socket.h>
],
[
int fd = -1;
struct sockaddr addr;
socklen_t addrlen;
(void) getsockname(fd, &addr, &addrlen);
], [
AC_MSG_RESULT([yes])
AC_DEFINE([GETSOCKNAME_TAKES_SOCKLEN_T], [], [Define if getsockname takes socklen_t as its third argument])],[
AC_MSG_RESULT([no])
])

# Checks for libraries.
AC_CHECK_LIB(pthread, pthread_self, [LIB_PTHREAD="-lpthread"], [LIB_PTHREAD=""])
AC_CHECK_LIB([unistr], [mbstowc16s], [LIBUNISTR_LIBS="-lunistr"], [], [$LIBUNISTR_LDFLAGS])
AC_CHECK_LIB([uuid], [uuid_generate_time], [LIBUUID_LIBS="-luuid"], [], [$LIBUUID_LDFLAGS])
AC_CHECK_LIB([rt], [clock_settime], [RT_LIBS="-lrt"])

AC_SUBST(LIB_PTHREAD)
AC_SUBST(LIBUNISTR_LIBS)
AC_SUBST(LIBUUID_LIBS)
AC_SUBST(RT_LIBS)

AC_CHECK_FUNCS([strndup strncasecmp])

old_LDFLAGS="$LDFLAGS"
LDFLAGS="$LDFLAGS $RT_LIBS"
AC_CHECK_FUNCS([clock_gettime clock_settime settimeofday gettimeofday timegm])
LDFLAGS="$old_LDFLAGS"

# Checks for header files.
AC_CHECK_HEADERS([string.h strings.h sys/types.h sys/socket.h sys/varargs.h pthread.h])

old_CFLAGS="$CFLAGS"
CFLAGS="$CFLAGS:$LIBUUID_INCLUDES"
AC_CHECK_HEADERS([uuid/uuid.h])
CFLAGS="$old_CFLAGS"

# Checks for typedefs, structures, and compiler characteristics.
AC_C_BIGENDIAN

# Checks for library functions.
AC_HEADER_STDC

AC_CONFIG_FILES([Makefile
                 include/Makefile
                 memory/Makefile
                 lwunistr/Makefile
                 join/Makefile])

AC_OUTPUT
