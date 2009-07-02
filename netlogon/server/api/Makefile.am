
noinst_LTLIBRARIES = liblwnetserverapi.la

liblwnetserverapi_la_CPPFLAGS = \
	-I@top_srcdir@/server/include \
	@LIBUNISTR_INCLUDES@          \
	@SQLITE_INCLUDES@             \
	@PSTORE_INCLUDES@             \
	@OPENLDAP_INCLUDES@

liblwnetserverapi_la_SOURCES = \
       globals.c          \
       lwnet-server-cfg.c \
       lwnet-ldap.c       \
       lwnet-krb5.c       \
       lwnet-cachedb.c    \
       lwnet-pstore.c     \
       lwnet-plugin.c     \
       dcinfo.c           \
       event.c            \
       lwnet.c            \
       state.c

liblwnetserverapi_la_LIBADD = \
		@LIBUNISTR_LIBS@      \
		@SQLITE_LIBS@         \
		@PSTORE_LIBS@         \
		@OPENLDAP_LIBS@       \
		@PTHREAD_LIBS@        \
		@top_builddir@/server/ipc/liblwnetserveripc.la

liblwnetserverapi_la_LDFLAGS = \
        @LIBUNISTR_LDFLAGS@    \
        @SQLITE_LDFLAGS@       \
        @PSTORE_LDFLAGS@       \
        @OPENLDAP_LDFLAGS@

noinst_HEADERS = \
		eventdlapi.h           \
		event_p.h              \
		evtstruct.h            \
		lwnet-cachedbcreate.h  \
		lwnet-cachedb.h        \
		lwnet-cachedb_p.h      \
		lwnet-krb5_p.h         \
		lwnet-ldap_p.h         \
		lwnet_p.h              \
		lwnet-pstore_p.h       \
		lwnet-server-cfg_p.h   \
		state_p.h
