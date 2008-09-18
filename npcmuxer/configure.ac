#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_INIT([named pipe client multiplexer], 1.0, support@centeris.com)
AM_INIT_AUTOMAKE([-Wall -Werror foreign])
CFLAGS="$CFLAGS -g -fno-strict-aliasing -Wall -Werror"
AC_CONFIG_SRCDIR([ctlib/test/testclient.c])
AC_CONFIG_HEADER([config.h])

case `uname` in
     FreeBSD|Isilon*)
         FEATUREMACRO_FLAGS="-D_XOPEN_SOURCE=600 -D_BSD_SOURCE=1 -D__BSD_VISIBLE -D_REENTRANT -D_THREAD_SAFE"
	 ;;
     *)
         FEATUREMACRO_FLAGS="-D_XOPEN_SOURCE=500 -D__EXTENSIONS__ -D_ALL_SOURCE=1 -D_POSIX_PTHREAD_SEMANTICS -D_REENTRANT -D_THREAD_SAFE"
	 ;;
esac

CPPFLAGS="$CPPFLAGS $FEATUREMACRO_FLAGS"

AC_SUBST(LTLDFLAGS)

SMBCLIENT_CPPFLAGS=""
SMBCLIENT_LDFLAGS=""
AC_ARG_WITH([smbclient-dir],
	       AC_HELP_STRING([--with-smbclient-dir=DIR],
	       [The base path where smbclient is installed in (usually /opt/dcerpc)]),
[dnl
case "$withval" in
yes|'') ;;
no) AC_MSG_ERROR([smbclient is required]) ;;
*) SMBCLIENT_CPPFLAGS="-I$withval/include"
SMBCLIENT_LDFLAGS="-L$withval/$(basename $libdir)" ;;
esac
])

AC_ARG_WITH([smbclient-libdir],
	       AC_HELP_STRING([--with-smbclient-libdir=DIR],
	       [The path where smbclient is installed in (defaults to --with-smbclient-dir/lib)]),
[dnl
SMBCLIENT_LDFLAGS="-L$withval"
])

KRB5_CPPFLAGS=""
KRB5_LDFLAGS=""
AC_ARG_WITH([krb5-dir],
	       AC_HELP_STRING([--with-krb5-dir=DIR],
	       [The prefix path where krb5 is installed]),
[dnl
KRB5_CPPFLAGS="-I$withval/include"
KRB5_LDFLAGS="-L$withval/`basename $libdir`"
])

AC_ARG_WITH([krb5-libdir],
	       AC_HELP_STRING([--with-krb5-libdir=DIR],
	       [The path where the krb5 library is installed (defaults to --with-krb5-dir/lib)]),
[dnl
KRB5_LDFLAGS="-L$withval"
])

NETLOGON_CPPFLAGS=""
NETLOGON_LDFLAGS=""
AC_ARG_WITH([netlogon-dir],
	       AC_HELP_STRING([--with-netlogon-dir=DIR],
	       [The prefix path where netlogon is installed]),
[dnl
NETLOGON_CPPFLAGS="-I$withval/include"
NETLOGON_LDFLAGS="-L$withval/`basename $libdir`"
])

AC_ARG_WITH([netlogon-libdir],
	       AC_HELP_STRING([--with-netlogon-libdir=DIR],
	       [The path where the netlogon library is installed (defaults to --with-netlogon-dir/lib)]),
[dnl
NETLOGON_LDFLAGS="-L$withval"
])

# Checks for programs.
AC_PROG_CC
AC_PROG_RANLIB
AM_PROG_CC_C_O
AC_PROG_LIBTOOL

# Checks for libraries.
AC_CHECK_LIB([c], [malloc])

AC_CHECK_LIB([krb5], [krb5_free_context], [KRB5_LDFLAGS="$KRB5_LDFLAGS -lkrb5 -lcom_err"], AC_MSG_ERROR([Use --with-krb5-dir to find libkrb5.so]), [$KRB5_LDFLAGS])

AC_CHECK_LIB([smbclient], [smbc_init_context], [SMBCLIENT_LDFLAGS="$SMBCLIENT_LDFLAGS -lsmbclient"], AC_MSG_ERROR([Use --with-smbclient-dir to specify the base path (everything except /lib) that contains smbclient]), [$SMBCLIENT_LDFLAGS $KRB5_LDFLAGS] )

AC_CHECK_LIB([lwnetclientapi], [LWNetExtendEnvironmentForKrb5Affinity], [NETLOGON_LDFLAGS="$NETLOGON_LDFLAGS -llwnetclientapi"], AC_MSG_ERROR([Use --with-netlogon-dir to find liblwnetclientapi.so]), [$NETLOGON_LDFLAGS])

AC_CHECK_LIB([pthread], [pthread_create])

AC_SUBST([SMBCLIENT_CPPFLAGS])
AC_SUBST([SMBCLIENT_LDFLAGS])

AC_SUBST([KRB5_CPPFLAGS])
AC_SUBST([KRB5_LDFLAGS])

AC_SUBST([NETLOGON_CPPFLAGS])
AC_SUBST([NETLOGON_LDFLAGS])

# Checks for header files.
AC_HEADER_DIRENT
AC_HEADER_STDC
AC_HEADER_SYS_WAIT
AC_CHECK_HEADERS([fcntl.h stdint.h stdlib.h string.h strings.h sys/socket.h syslog.h termios.h unistd.h select.h sys/select.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_HEADER_STDBOOL
# The AC_C_CONST macro is broken on Linux. Don't use it.
AC_TYPE_UID_T
AC_TYPE_MODE_T
AC_TYPE_PID_T
AC_TYPE_SIZE_T
AC_HEADER_TIME
AC_STRUCT_TM
AC_C_BIGENDIAN(,,[AC_MSG_ERROR(System has unknown byte order)])

# Checks for library functions.
AC_FUNC_CHOWN
AC_FUNC_CLOSEDIR_VOID
AC_FUNC_FORK
AC_FUNC_LSTAT
AC_FUNC_LSTAT_FOLLOWS_SLASHED_SYMLINK
AC_FUNC_MALLOC
AC_FUNC_MEMCMP
AC_FUNC_REALLOC
AC_FUNC_SELECT_ARGTYPES
AC_TYPE_SIGNAL
AC_FUNC_STAT
AC_FUNC_STRFTIME
AC_FUNC_VPRINTF
AC_CHECK_FUNCS([atexit dup2 getcwd localtime_r memmove memset mkdir realpath regcomp rmdir select socket strerror strncasecmp])
AC_CHECK_FUNCS([sigwait sigwaitinfo])
AC_CHECK_FUNCS([rand random])
AC_CHECK_FUNCS([vsyslog])

AC_CHECK_LIB(socket, socket, [LIB_socket="-lsocket"], [LIB_socket=""])
AC_SUBST(LIB_socket)

AC_CONFIG_FILES([Makefile
                 ctlib/Makefile
                 ctlib/lib/Makefile
                 ctlib/test/Makefile
                 npclib/Makefile
                 npclib/lib/Makefile
                 npclib/test/Makefile
                 npclib/include/Makefile
                 server/Makefile])
AC_OUTPUT
