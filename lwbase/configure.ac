AC_INIT(lwbase, 0.1, support@likewisesoftware.com)
AM_INIT_AUTOMAKE([-Wall -Werror foreign])
AC_CONFIG_HEADER([include/config.h])
AC_CANONICAL_HOST

LWBASE_CPPFLAGS=""
LWBASE_CFLAGS="-Wall -Werror -Wmissing-prototypes"
LWBASE_LDFLAGS=""

case "$host_os:$host_cpu" in
        solaris*:*)
                LWBASE_CPPFLAGS="$LWBASE_CPPFLAGS -D_XOPEN_SOURCE=500 -D_EXTENSIONS -D_REENTRANT"
		;;
	hpux*:hppa*)
		LWBASE_CPPFLAGS="$LWBASE_CPPFLAGS -D_XOPEN_SOURCE_EXTENDED=1 -D_XOPEN_SOURCE=500"
		;;
	hpux*:ia64*)
		LWBASE_CPPFLAGS="$LWBASE_CPPFLAGS -D_XOPEN_SOURCE_EXTENDED=1 -D_XOPEN_SOURCE=500"
		;;
	aix*:*)
		LWBASE_CPPFLAGS="$LWBASE_CPPFLAGS -D_THREAD_SAFE"
		;;	
esac

AC_SUBST(LWBASE_CPPFLAGS)
AC_SUBST(LWBASE_CFLAGS)
AC_SUBST(LWBASE_LDFLAGS)

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

LIBUNISTR_LIBS="-lunistr"

AC_SUBST(LIBUNISTR_INCLUDES)
AC_SUBST(LIBUNISTR_LDFLAGS)
AC_SUBST(LIBUNISTR_LIBS)

# Checks for libraries.
AC_CHECK_LIB(pthread, pthread_self, [LIB_PTHREAD="-lpthread"], [LIB_PTHREAD=""])
AC_CHECK_LIB([unistr], [mbstowc16s], [LIBUNISTR_LIBS="-lunistr"], [], [$LIBUNISTR_LDFLAGS])

# Checks for header files.
AC_CHECK_HEADERS([string.h strings.h sys/types.h sys/socket.h sys/varargs.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_C_BIGENDIAN

# Checks for library functions.
AC_HEADER_STDC

AC_CONFIG_FILES([Makefile include/Makefile include/lw/Makefile src/Makefile])

AC_OUTPUT
