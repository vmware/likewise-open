#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ(2.60)
AC_INIT([libsmbclient], [3.0.28], [support@likewisesoftware.com])
AM_INIT_AUTOMAKE([foreign])
AC_CONFIG_SRCDIR([libsmb/libsmbclient.c])
AC_CONFIG_HEADER([include/config.h])
AC_DEFINE(CONFIG_H_IS_FROM_SAMBA,1,[Marker for samba's config.h])
AC_DEFINE(HAVE_KRB5,1,[Indicates that kerberos should be enabled])
AC_DEFINE(HAVE_MAGIC_IN_KRB5_ADDRESS,1,[Indicates that kerberos should be enabled])
AC_DEFINE(HAVE_ADDRTYPE_IN_KRB5_ADDRESS,1,[Indicates that kerberos should be enabled])
AC_DEFINE(HAVE_SHORT_KRB5_MK_ERROR_INTERFACE,1,[Indicates that kerberos should be enabled])
configdir='${libdir}'
AC_SUBST(configdir)

# Checks for programs.
AC_PROG_CC
AC_USE_SYSTEM_EXTENSIONS
AC_PROG_LIBTOOL

# Check for compiler options
AC_MSG_CHECKING([if linker supports -rpath-link])
saved_LDFLAGS="$LDFLAGS"
LDFLAGS="$LDFLAGS -Wl,-rpath-link=/usr/lib"

AC_TRY_LINK([], [],
      [ AC_MSG_RESULT([yes]); RPATH_LINK=yes ],
      [ AC_MSG_RESULT([no]); RPATH_LINK=no ])

LDFLAGS="$saved_LDFLAGS"

LDAP_CPPFLAGS=""
AC_ARG_WITH([ldap-dir],
	       AC_HELP_STRING([--with-ldap-dir=DIR],
	       [The prefix path where ldap is installed]),
[dnl
LDAP_CPPFLAGS="-I$withval/include"
LDAP_LIBDIR="$withval/`basename $libdir`"
])

AC_ARG_WITH([ldap-libdir],
	       AC_HELP_STRING([--with-ldap-libdir=DIR],
	       [The path where the ldap library is installed (defaults to --with-ldap-dir/lib)]),
[dnl
LDAP_LIBDIR="$withval"
])
CPPFLAGS="$CPPFLAGS $LDAP_CPPFLAGS"
LDAP_LDFLAGS=""
if test -n "$LDAP_LIBDIR"; then
	if test "x$RPATH_LINK" = "xyes"; then
		LDAP_LDFLAGS="-L$LDAP_LIBDIR -Wl,-rpath-link=$LDAP_LIBDIR"
	else
		LDAP_LDFLAGS="-L$LDAP_LIBDIR"
	fi
fi
AC_SUBST(LDAP_LDFLAGS)

KRB5_CPPFLAGS=""
AC_ARG_WITH([krb5-dir],
	       AC_HELP_STRING([--with-krb5-dir=DIR],
	       [The prefix path where krb5 is installed]),
[dnl
KRB5_CPPFLAGS="-I$withval/include"
KRB5_LIBDIR="$withval/`basename $libdir`"
])

AC_ARG_WITH([krb5-libdir],
	       AC_HELP_STRING([--with-krb5-libdir=DIR],
	       [The path where the krb5 library is installed (defaults to --with-krb5-dir/lib)]),
[dnl
KRB5_LIBDIR="$withval"
])
CPPFLAGS="$CPPFLAGS $KRB5_CPPFLAGS"
KRB5_LDFLAGS=""
if test -n "$KRB5_LIBDIR"; then
	if test "x$RPATH_LINK" = "xyes"; then
		KRB5_LDFLAGS="-L$KRB5_LIBDIR -Wl,-rpath-link=$KRB5_LIBDIR"
	else
		KRB5_LDFLAGS="-L$KRB5_LIBDIR"
	fi
fi

# Checks for libraries.
AC_CHECK_LIB([nsl], [gethostbyname])
AC_CHECK_LIB([socket], [connect])

AC_CHECK_LIB([krb5support], [krb5int_free_error], [KRB5_LDFLAGS="$KRB5_LDFLAGS -lkrb5support"], AC_MSG_ERROR([Use --with-krb5-dir to find libkrb5support.so]), [$KRB5_LDFLAGS])

AC_CHECK_LIB([k5crypto], [krb5_c_string_to_key], [KRB5_LDFLAGS="$KRB5_LDFLAGS -lk5crypto"], AC_MSG_ERROR([Use --with-krb5-dir to find libk5crypto.so]), [$KRB5_LDFLAGS])

AC_CHECK_LIB([krb5], [krb5_init_context], [KRB5_LDFLAGS="$KRB5_LDFLAGS -lkrb5 -lcom_err"], AC_MSG_ERROR([Use --with-krb5-dir to find libkrb5.so]), [$KRB5_LDFLAGS])
AC_SUBST(KRB5_LDFLAGS)

AC_CHECK_LIB([ldap], [ldap_init], [
LDAP_LDFLAGS="$LDAP_LDFLAGS -lldap"
], AC_MSG_ERROR([Use --with-ldap-dir to find libldap.so]), [$LDAP_LDFLAGS])
AC_SUBST(LDAP_LDFLAGS)

# Checks for header files.

case "$host_os" in
	*hpux*)
		if test "`uname -r`" = "B.11.23"; then
			AC_MSG_WARN([Enabling HPUX 11.23 machine/sys/getppdp.h bug workaround])
			CFLAGS="$CFLAGS -D_MACHINE_SYS_GETPPDP_INCLUDED"
		fi
		;;
esac
AC_HEADER_RESOLV
AC_HEADER_STDC
AC_CHECK_HEADERS([alloc.h arpa/inet.h attr/xattr.h com_err.h crypt.h dirent.h grp.h gssapi.h inttypes.h krb5.h lber.h ldap.h libaio.h limits.h locale.h memory.h ndir.h netinet.h netinet/in.h netinet/ip.h netinet/in_ip.h netinet/in_systm.h netinet/tcp.h pwd.h readline.h resolv.h setjmp.h shadow.h standards.h stdint.h stdlib.h string.h strings.h sys/attributes.h sys/capability.h sys/dir.h sys/fcntl.h sys/ioctl.h syslog.h sys/mman.h sys/mode.h sys/mount.h sys/ndir.h sys/security.h sys/socket.h sys/syslog.h sys/time.h sys/types.h sys/un.h sys/wait.h termios.h unistd.h utime.h wchar.h sys/sockio.h net/if.h sys/param.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_DEFINE_UNQUOTED(DEFAULT_UNIX_CHARSET,"UTF8",[Default unix charset name])
AC_DEFINE_UNQUOTED(DEFAULT_DOS_CHARSET,"ASCII",[Default dos charset name])
AC_DEFINE_UNQUOTED(DEFAULT_DISPLAY_CHARSET,"ASCII",[Default display charset name])

AC_C_CONST
AC_HEADER_STDBOOL
AC_TYPE_UID_T
AC_C_INLINE
AC_TYPE_INT16_T
AC_TYPE_INT32_T
AC_TYPE_INT64_T
AC_TYPE_INT8_T
AC_TYPE_MODE_T
AC_TYPE_OFF_T
AC_TYPE_PID_T
AC_TYPE_SIZE_T
AC_TYPE_SSIZE_T
AC_CHECK_MEMBERS([struct stat.st_blksize])
AC_STRUCT_ST_BLOCKS
AC_CHECK_MEMBERS([struct stat.st_rdev])
AC_HEADER_TIME
AC_STRUCT_TM
AC_TYPE_UINT16_T
AC_TYPE_UINT32_T
AC_TYPE_UINT64_T
AC_TYPE_UINT8_T
AC_C_VOLATILE
AC_CHECK_TYPES([ptrdiff_t, socklen_t, struct timespec, sig_atomic_t, comparison_fn_t, uint, u_char, u_int, u_short, uint32, uint8, u_long], , , [
#if HAVE_STDLIB_H
#include <stdlib.h>
#endif
#if HAVE_SYS_WAIT_H
#include <sys/wait.h>
#endif
#if HAVE_UNISTD_H
#include <unistd.h>
#endif
#if HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif
#if HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#if HAVE_INTTYPES_H
#include <inttypes.h>
#endif
#if HAVE_STDINT_H
#include <stdint.h>
#endif
#ifdef TIME_WITH_SYS_TIME
#include <sys/time.h>
#include <time.h>
#else
#if HAVE_SYS_TIME_H
#include <sys/time.h>
#endif
#if HAVE_TIME_H
#include <time.h>
#endif
#endif
		])

AC_CHECK_SIZEOF(int)
AC_CHECK_SIZEOF(long)
AC_CHECK_SIZEOF(long long)
AC_CHECK_SIZEOF(short)

# Checks for library functions.
AC_FUNC_CHOWN
AC_FUNC_CLOSEDIR_VOID
AC_REPLACE_FNMATCH
AC_FUNC_FORK
AC_FUNC_GETGROUPS
AC_PROG_GCC_TRADITIONAL
AC_FUNC_LSTAT
AC_FUNC_LSTAT_FOLLOWS_SLASHED_SYMLINK
AC_HEADER_MAJOR
AC_FUNC_MEMCMP
AC_FUNC_MMAP
AC_FUNC_SELECT_ARGTYPES
AC_FUNC_SETVBUF_REVERSED
AC_TYPE_SIGNAL
AC_FUNC_STAT
AC_FUNC_STRFTIME
AC_FUNC_UTIME_NULL
AC_CHECK_FUNCS([alarm atexit dup2 endgrent endpwent ftruncate getcwd gethostbyaddr gethostbyname gethostname getpagesize getpass gettimeofday inet_ntoa memmove memset mkdir munmap nl_langinfo pow putenv realpath select setenv setlocale socket strcasecmp strchr strcspn strdup strerror strncasecmp strndup strpbrk strrchr strspn strstr strtol strtoul strtoull __strtoull tzset utime strtok_r initgroups chsize strlcpy strlcat mktime vsyslog syslog strnlen strndup waitpid seteuid setegid chroot pread pwrite strcasestr unsetenv socketpair timegm strtoll __strtoll strtoq mkstemp mkdtemp sigprocmask sigblock bzero snprintf vsnprintf setresuid setresgid srandom srand random rand nanosleep dlerror dlopen dlclose dlsym])
old_LDFLAGS="$LDFLAGS"
LDFLAGS="$LDFLAGS $KRB5_LDFLAGS"
AC_CHECK_FUNCS([krb5_set_real_time krb5_principal2salt krb5_use_enctype krb5_string_to_key krb5_encrypt_block krb5_get_pw_salt krb5_string_to_key_salt krb5_get_permitted_enctypes krb5_get_default_in_tkt_etypes krb5_auth_con_setkey krb5_auth_con_setuseruserkey krb5_c_string_to_key krb5_free_keytab_entry_contents krb5_c_verify_checksum krb5_get_renewed_creds])
LDFLAGS="$old_LDFLAGS"
AC_CHECK_MEMBERS([krb5_ticket.enc_part2, krb5_creds.keyblock, krb5_keytab_entry.key],
, ,
[AC_INCLUDES_DEFAULT
#if HAVE_KRB5_H
#include <krb5.h>
#endif])
AC_CHECK_DECLS([AP_OPTS_USE_SUBKEY, va_copy], , , [#if HAVE_KRB5_H
#include <krb5.h>
#endif])
AC_CACHE_CHECK([if gettimeofday takes tz argument],samba_cv_HAVE_GETTIMEOFDAY_TZ,[
AC_TRY_LINK([
#include <sys/time.h>
#include <unistd.h>], [struct timeval tv; return gettimeofday(&tv, NULL);],
           samba_cv_HAVE_GETTIMEOFDAY_TZ=yes,
	   samba_cv_HAVE_GETTIMEOFDAY_TZ=no)])
if test x"$samba_cv_HAVE_GETTIMEOFDAY_TZ" = x"yes"; then
    AC_DEFINE(HAVE_GETTIMEOFDAY_TZ,1,[Whether gettimeofday takes a tz argument])
fi

AC_CACHE_CHECK([for secure mkstemp],samba_cv_HAVE_SECURE_MKSTEMP,[
AC_TRY_RUN([#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
main() {
  struct stat st;
  char tpl[20]="/tmp/test.XXXXXX";
  int fd = mkstemp(tpl);
  if (fd == -1) exit(1);
  unlink(tpl);
  if (fstat(fd, &st) != 0) exit(1);
  if ((st.st_mode & 0777) != 0600) exit(1);
  exit(0);
}],
samba_cv_HAVE_SECURE_MKSTEMP=yes,
samba_cv_HAVE_SECURE_MKSTEMP=no,
samba_cv_HAVE_SECURE_MKSTEMP=cross)])
if test x"$samba_cv_HAVE_SECURE_MKSTEMP" = x"yes"; then
    AC_DEFINE(HAVE_SECURE_MKSTEMP,1,[Whether mkstemp is secure])
fi

##################
# look for a method of finding the list of network interfaces
iface=no;
case $host_os in
	*aix*)
		AC_CACHE_CHECK([for iface AIX],samba_cv_HAVE_IFACE_AIX,[
		SAVE_CPPFLAGS="$CPPFLAGS"
			CPPFLAGS="$CPPFLAGS ${SAMBA_CONFIGURE_CPPFLAGS} -Iinclude"
		AC_TRY_RUN([
		#define NO_CONFIG_H 1
		#define HAVE_IFACE_AIX 1
		#define AUTOCONF_TEST 1
		#undef _XOPEN_SOURCE_EXTENDED
		#include "${srcdir-.}/lib/replace/replace.c"
		#include "${srcdir-.}/lib/replace/snprintf.c"
		#include "${srcdir-.}/lib/interfaces.c"],
		           samba_cv_HAVE_IFACE_AIX=yes,samba_cv_HAVE_IFACE_AIX=no,samba_cv_HAVE_IFACE_AIX=cross)])
		CPPFLAGS="$SAVE_CPPFLAGS"
		if test x"$samba_cv_HAVE_IFACE_AIX" = x"yes"; then
		    iface=yes;AC_DEFINE(HAVE_IFACE_AIX,1,[Whether iface AIX is available])
		fi
		;;
esac

if test $iface = no; then
AC_CACHE_CHECK([for iface getifaddrs],samba_cv_HAVE_IFACE_GETIFADDRS,[
SAVE_CPPFLAGS="$CPPFLAGS"
CPPFLAGS="$CPPFLAGS ${SAMBA_CONFIGURE_CPPFLAGS} -Iinclude"
AC_TRY_RUN([
#define NO_CONFIG_H 1
#define HAVE_IFACE_GETIFADDRS 1
#define AUTOCONF_TEST 1
#include "${srcdir-.}/lib/replace/replace.c"
#include "${srcdir-.}/lib/replace/snprintf.c"
#include "${srcdir-.}/lib/interfaces.c"],
           samba_cv_HAVE_IFACE_GETIFADDRS=yes,samba_cv_HAVE_IFACE_GETIFADDRS=no,samba_cv_HAVE_IFACE_GETIFADDRS=cross)])
CPPFLAGS="$SAVE_CPPFLAGS"
if test x"$samba_cv_HAVE_IFACE_GETIFADDRS" = x"yes"; then
    iface=yes;AC_DEFINE(HAVE_IFACE_GETIFADDRS,1,[Whether iface getifaddrs is available])
fi
fi

if test $iface = no; then
AC_CACHE_CHECK([for iface ifconf],samba_cv_HAVE_IFACE_IFCONF,[
SAVE_CPPFLAGS="$CPPFLAGS"
CPPFLAGS="$CPPFLAGS ${SAMBA_CONFIGURE_CPPFLAGS} -Iinclude"
AC_TRY_RUN([
#define NO_CONFIG_H 1
#define HAVE_IFACE_IFCONF 1
#define AUTOCONF_TEST 1
#define SOCKET_WRAPPER_NOT_REPLACE
#include "${srcdir-.}/lib/replace/replace.c"
#include "${srcdir-.}/lib/replace/snprintf.c"
#include "${srcdir-.}/lib/interfaces.c"],
           samba_cv_HAVE_IFACE_IFCONF=yes,samba_cv_HAVE_IFACE_IFCONF=no,samba_cv_HAVE_IFACE_IFCONF=cross)])
CPPFLAGS="$SAVE_CPPFLAGS"
if test x"$samba_cv_HAVE_IFACE_IFCONF" = x"yes"; then
    iface=yes;AC_DEFINE(HAVE_IFACE_IFCONF,1,[Whether iface ifconf is available])
fi
fi

if test $iface = no; then
AC_CACHE_CHECK([for iface ifreq],samba_cv_HAVE_IFACE_IFREQ,[
SAVE_CPPFLAGS="$CPPFLAGS"
CPPFLAGS="$CPPFLAGS ${SAMBA_CONFIGURE_CPPFLAGS} -Iinclude"
AC_TRY_RUN([
#define NO_CONFIG_H 1
#define HAVE_IFACE_IFREQ 1
#define AUTOCONF_TEST 1
#define SOCKET_WRAPPER_NOT_REPLACE
#include "${srcdir-.}/lib/replace/replace.c"
#include "${srcdir-.}/lib/replace/snprintf.c"
#include "${srcdir-.}/lib/interfaces.c"],
           samba_cv_HAVE_IFACE_IFREQ=yes,samba_cv_HAVE_IFACE_IFREQ=no,samba_cv_HAVE_IFACE_IFREQ=cross)])
CPPFLAGS="$SAVE_CPPFLAGS"
if test x"$samba_cv_HAVE_IFACE_IFREQ" = x"yes"; then
    iface=yes;AC_DEFINE(HAVE_IFACE_IFREQ,1,[Whether iface ifreq is available])
fi
fi

if test $iface = no; then
AC_CACHE_CHECK([for iface AIX],samba_cv_HAVE_IFACE_AIX,[
SAVE_CPPFLAGS="$CPPFLAGS"
CPPFLAGS="$CPPFLAGS ${SAMBA_CONFIGURE_CPPFLAGS} -Iinclude"
AC_TRY_RUN([
#define NO_CONFIG_H 1
#define HAVE_IFACE_AIX 1
#define AUTOCONF_TEST 1
#undef _XOPEN_SOURCE_EXTENDED
#define SOCKET_WRAPPER_NOT_REPLACE
#include "${srcdir-.}/lib/replace/replace.c"
#include "${srcdir-.}/lib/replace/snprintf.c"
#include "${srcdir-.}/lib/interfaces.c"],
           samba_cv_HAVE_IFACE_AIX=yes,samba_cv_HAVE_IFACE_AIX=no,samba_cv_HAVE_IFACE_AIX=cross)])
CPPFLAGS="$SAVE_CPPFLAGS"
if test x"$samba_cv_HAVE_IFACE_AIX" = x"yes"; then
    iface=yes;AC_DEFINE(HAVE_IFACE_AIX,1,[Whether iface AIX is available])
fi
fi

AC_CONFIG_FILES([Makefile])
AC_CONFIG_FILES([librpc/Makefile])
AC_OUTPUT
