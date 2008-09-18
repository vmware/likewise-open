#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ(2.59)
AC_INIT(libunistr, 1.0, support@centeris.com)
AM_INIT_AUTOMAKE([foreign])
AC_CONFIG_SRCDIR([wc16str.c])
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

AC_CONFIG_FILES([Makefile])

AC_OUTPUT
