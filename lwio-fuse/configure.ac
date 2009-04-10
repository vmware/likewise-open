AC_INIT(lwio-fuse, 0.1, support@likewisesoftware.com)
AM_INIT_AUTOMAKE([-Wall -Werror foreign])
AC_CONFIG_HEADER([include/config.h])

LWIO_FUSE_CPPFLAGS="-D_FILE_OFFSET_BITS=64"
LWIO_FUSE_CFLAGS="-Wall -Werror -Wmissing-prototypes"
LWIO_FUSE_LDFLAGS=""

AC_SUBST(LWIO_FUSE_CPPFLAGS)
AC_SUBST(LWIO_FUSE_CFLAGS)
AC_SUBST(LWIO_FUSE_LDFLAGS)
AC_SUBST(LIB_LWIOCLIENT)
AC_SUBST(LIB_LWBASE)
AC_SUBST(LIB_LSACLIENT)
AC_SUBST(LIB_FUSE)

# Checks for programs.
AC_PROG_CC
AC_GNU_SOURCE
AC_PROG_INSTALL
AC_PROG_MKDIR_P
AC_PROG_LIBTOOL

# Checks for libraries.
AC_CHECK_LIB(lwioclient, LwIoOpenContextShared, [LIB_LWIOCLIENT="-llwioclient"], [AC_MSG_ERROR([liblwioclient not found])])
AC_CHECK_LIB(lwbase, LwNtStatusToUnixErrno, [LIB_LWBASE="-llwbase"], [AC_MSG_ERROR([liblwbase not found])])
AC_CHECK_LIB(lsaclient, LsaOpenServer, [LIB_LSACLIENT="-llsaclient"], [AC_MSG_ERROR([liblsaclient not found])])
AC_CHECK_LIB(fuse, fuse_new, [LIB_FUSE="-lfuse"], [AC_MSG_ERROR([libfuse not found])])

# Checks for header files.
AC_CHECK_HEADERS([string.h strings.h sys/types.h sys/varargs.h])
AC_CHECK_HEADERS([lwio/lwio.h lw/base.h fuse.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST
AC_C_INLINE
AC_TYPE_SIZE_T

# Checks for library functions.
AC_HEADER_STDC

AC_CONFIG_FILES([Makefile include/Makefile src/Makefile])

AC_OUTPUT
