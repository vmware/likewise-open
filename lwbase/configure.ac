AC_INIT(lwbase, 0.1, support@likewisesoftware.com)
AM_INIT_AUTOMAKE([-Wall -Werror foreign])
AC_CONFIG_HEADER([include/config.h])
AC_CANONICAL_HOST

LWBASE_CPPFLAGS=""
LWBASE_CFLAGS="-Wall -Werror -Wmissing-prototypes"
LWBASE_LDFLAGS=""

CPPFLAGS="$CPPFLAGS -D_REENTRANT -D_GNU_SOURCE"

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

LIBUUID_LIBS="-luuid"

AC_SUBST(LIBUUID_INCLUDES)
AC_SUBST(LIBUUID_LDFLAGS)
AC_SUBST(LIBUUID_LIBS)


# Checks for libraries.
AC_CHECK_LIB(pthread, pthread_self, [LIB_PTHREAD="-lpthread"], [LIB_PTHREAD=""])
AC_CHECK_LIB([unistr], [mbstowc16s], [LIBUNISTR_LIBS="-lunistr"], [], [$LIBUNISTR_LDFLAGS])
AC_CHECK_LIB([uuid], [uuid_generate_time], [LIBUUID_LIBS="-luuid"], [], [$LIBUUID_LDFLAGS])

AC_SUBST(LIB_PTHREAD)

AC_CHECK_FUNCS([strndup])

# Checks for header files.
AC_CHECK_HEADERS([string.h strings.h sys/types.h sys/socket.h sys/varargs.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_C_BIGENDIAN

# Checks for library functions.
AC_HEADER_STDC

# Check for moonunit bits
AC_CHECK_HEADERS([moonunit/moonunit.h], [HAVE_MOONUNIT_H=true], [HAVE_MOONUNIT_H=false])
AC_CHECK_LIB(moonunit, Mu_Interface_Result, [HAVE_MOONUNIT_LIB=true], [HAVE_MOONUNIT_LIB=false])
AC_CHECK_PROG(MOONUNIT_STUB, moonunit-stub, moonunit-stub, false)

if test "${MOONUNIT_STUB}" = "moonunit-stub" && $HAVE_MOONUNIT_H && $HAVE_MOONUNIT_LIB
then
	HAVE_MOONUNIT="true"
	LIB_MOONUNIT="-lmoonunit"
	AC_DEFINE([HAVE_MOONUNIT], [], [Have MoonUnit])
else
	HAVE_MOONUNIT="false"
fi

AC_SUBST(LIB_MOONUNIT)
AM_CONDITIONAL(HAVE_MOONUNIT, $HAVE_MOONUNIT)

AC_CONFIG_FILES([Makefile
                 include/Makefile
                 include/lw/Makefile
                 include/secdesc/Makefile
                 src/Makefile
                 moonunit/Makefile])

AC_OUTPUT
