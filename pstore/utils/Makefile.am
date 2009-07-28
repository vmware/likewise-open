AM_CPPFLAGS = -I$(top_srcdir)/include

noinst_LTLIBRARIES=liblwps-utils.la

liblwps_utils_la_SOURCES = \
       lwps-futils.c   \
       lwps-cfg.c      \
       lwps-error.c    \
       lwps-logger.c   \
       lwps-mem.c      \
       lwps-paths.c    \
       lwps-rwlock.c   \
       lwps-sid.c      \
       lwps-stack.c    \
       lwps-str.c      \
       lwps-strerror.c \
       lwps-sysfuncs.c

liblwps_utils_la_CPPFLAGS = @LIBUNISTR_INCLUDES@ $(AM_CPPFLAGS)
liblwps_utils_la_LDFLAGS = @LIBUNISTR_LDFLAGS@
liblwps_utils_la_LIBADD = @LIBUNISTR_LIBS@

noinst_HEADERS = \
	lwps-cfg_p.h \
	lwps-error_p.h \
	lwps-rwlock_p.h \
	lwps-sid_p.h
