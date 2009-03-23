#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ(2.59)
AC_INIT(libunistr, 1.0, support@centeris.com)
AM_INIT_AUTOMAKE([foreign])
AC_CONFIG_SRCDIR([lib/wc16str.c])
AC_CONFIG_HEADER([config.h])

AC_C_BIGENDIAN

# Checks for programs.
AC_PROG_CC
AC_PROG_LIBTOOL

# Checks for libraries.

# Checks for header files.

# Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST
AC_TYPE_SIZE_T
AC_CHECK_SIZEOF(wchar_t)

# Checks for library functions.
#AC_FUNC_MALLOC
AC_HEADER_STDC

AC_CHECK_HEADERS([wctype.h])

AC_CHECK_LIB(iconv, iconv, [LIB_iconv="-liconv"],
    AC_CHECK_LIB(iconv, libiconv, [LIB_iconv="-liconv"], [LIB_iconv=""]))

AC_SUBST(LIB_iconv)

saved_CFLAGS="$CFLAGS"
CFLAGS="$CFLAGS -Wall -Werror"

AC_MSG_CHECKING([whether iconv() takes const char** as its second parameter])

AC_TRY_COMPILE([#include <iconv.h>],
		[iconv_t h = iconv_open(NULL, NULL);
		 char* in;
		 (void) iconv(h, (const char**) &in, NULL, NULL, NULL);
		],
		[AC_MSG_RESULT([yes]); AC_DEFINE_UNQUOTED(ICONV_IN_TYPE, [const char**], [iconv in type])],
		[AC_MSG_RESULT([no]); AC_DEFINE_UNQUOTED(ICONV_IN_TYPE, [char**], [iconv in type])])

CFLAGS="$saved_CFLAGS"

AC_CHECK_FUNCS([wcscasecmp])

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
		 lib/Makefile
                 moonunit/Makefile])

AC_OUTPUT
