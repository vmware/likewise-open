AC_INIT(lwmapsecurity, 0.1, support@likewisesoftware.com)
AM_INIT_AUTOMAKE([-Wall -Werror foreign])
AC_CONFIG_HEADER([include/config.h])
AC_CONFIG_MACRO_DIR([m4])

AM_CPPFLAGS=""
AM_CFLAGS="-Wall -Werror -Wmissing-prototypes"
AM_LDFLAGS=""

CPPFLAGS="$CPPFLAGS -D_REENTRANT -D_GNU_SOURCE"

# Platform-specific stuff

MOD_EXT=".so"
case "$host_os:$host_cpu" in
        solaris*:*)
                AM_CPPFLAGS="$AM_CPPFLAGS -D_XOPEN_SOURCE=500 -D_EXTENSIONS -D_REENTRANT"
		;;
	hpux*:hppa*)
		MOD_EXT=".sl"
		AM_CPPFLAGS="$AM_CPPFLAGS -D_XOPEN_SOURCE_EXTENDED=1 -D_XOPEN_SOURCE=500"
		;;
	hpux*:ia64*)
		AM_CPPFLAGS="$AM_CPPFLAGS -D_XOPEN_SOURCE_EXTENDED=1 -D_XOPEN_SOURCE=500"
		;;
	aix*:*)
		AM_CPPFLAGS="$AM_CPPFLAGS -D_THREAD_SAFE"
		;;	
esac

AC_SUBST(AM_CPPFLAGS)
AC_SUBST(AM_CFLAGS)
AC_SUBST(AM_LDFLAGS)

AC_SUBST(MOD_EXT)
AC_DEFINE_UNQUOTED([MOD_EXT], ["$MOD_EXT"], [Extension of shared modules])

AS_AC_EXPAND(LIBDIR, $libdir)
AC_DEFINE_UNQUOTED(LIBDIR, "$LIBDIR", [Library directory])

# Checks for programs.
AC_PROG_CC
AC_PROG_LIBTOOL

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

# Checks for libraries.
AC_CHECK_LIB(pthread, pthread_self, [LIB_PTHREAD="-lpthread"], [LIB_PTHREAD=""])

AC_CHECK_LIB([dl], [dlopen], [DL_LIBS="-ldl"])
AC_SUBST(DL_LIBS)

# Checks for header files.
AC_CHECK_HEADERS([dlfcn.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_C_BIGENDIAN

# Checks for library functions.
AC_HEADER_STDC

AC_CONFIG_FILES([Makefile
                 include/Makefile
                 src/Makefile
                 test/Makefile])

AC_OUTPUT
