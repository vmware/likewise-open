AM_CPPFLAGS = -I$(top_srcdir)/include

noinst_LTLIBRARIES=liblwnetutils.la

liblwnetutils_la_SOURCES = \
        globals.c           \
	lwnet-cfg.c         \
	lwnet-dlinkedlist.c \
	lwnet-dns.c         \
	lwnet-error.c       \
	lwnet-futils.c      \
	lwnet-info.c        \
	lwnet-logger.c      \
	lwnet-mem.c         \
	lwnet-paths.c       \
	lwnet-stack.c       \
	lwnet-str.c         \
	lwnet-strerror.c    \
	lwnet-time.c        \
	lwnet-sysfuncs.c    \
	lwnet-xnet.c

noinst_HEADERS = \
        externs.h       \
        includes.h      \
	lwnet-cfg_p.h   \
	lwnet-dns_p.h   \
	lwnet-error_p.h

liblwnetutils_la_LDFLAGS =  \
	@LIBUNISTR_LIBS@    \
	@RESOLV_LIBS@       \
	@RT_LIBS@           \
	@PTHREAD_LIBS@

