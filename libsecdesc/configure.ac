AC_INIT(include/secdesc/sid.h)
AM_INIT_AUTOMAKE(libsecdesc,0.1)
AC_CONFIG_HEADER([include/config.h])

AC_PROG_CC
AC_PROG_LIBTOOL

UNISTR_INCLUDES=""
UNISTR_CFLAGS=""
UNISTR_LDFLAGS=""

AC_ARG_WITH(
	unistr-dir, 
	[  --with-unistr-dir=<prefix>       look for dcerpc headers/libraries in <prefix>],
	[
	    UNISTR_INCLUDES="-I$withval/include"
	    UNISTR_LDFLAGS="-L$withval/lib"
	]
)

AC_ARG_WITH(
	unistr-libdir, 
	[  --with-unistr-libdir=<libdir>    look for dcerpc libraries in <libdir>],
	[
	    UNISTR_LDFLAGS="-L$withval"
	]
)

AC_ARG_WITH(
	unistr-incdir,
	[  --with-unistr-incdir=<incdir>    look for dcerpc headers in <incdir>],
	[
	    UNISTR_INCLUDES="-I$withval/include"
	]
)

UNISTR_LIBS="-lunistr"

AC_SUBST(UNISTR_INCLUDES)
AC_SUBST(UNISTR_CFLAGS)
AC_SUBST(UNISTR_LDFLAGS)
AC_SUBST(UNISTR_LIBS)

CPPFLAGS="$CPPFLAGS $UNISTR_INCLUDES"
CFLAGS="$CFLAGS $UNISTR_CFLAGS $UNISTR_INCLUDES"

AC_ARG_ENABLE([unicode],
              [AC_HELP_STRING([--disable-unicode], [disable unicode support (default: enabled)])],
              [use_unicode=$enableval],
              [use_unicode=yes])

if test x"$use_unicode" = x"yes"; then
    AM_CFLAGS="$AM_CFLAGS -DUNICODE"
fi

AC_HEADER_STDBOOL
AC_CHECK_HEADERS([stdint.h stdlib.h inttypes.h wchar16.h uuid.h uuid/uuid.h])

AC_CHECK_LIB([uuid], [uuid_generate], [UUID_LIBS="-luuid"])
AC_SUBST(UUID_LIBS)

AM_CFLAGS="$AM_CFLAGS -Wall -Werror -fno-strict-aliasing"
AM_LDFLAGS=""

AC_SUBST(AM_CFLAGS)
AC_SUBST(AM_LDFLAGS)

AC_OUTPUT(Makefile
          include/Makefile
          src/Makefile
          test/Makefile)
