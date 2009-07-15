noinst_LTLIBRARIES = liblwnetserverapi.la

liblwnetserverapi_la_CPPFLAGS = \
	-I@top_srcdir@/server/include \
	@LIBUNISTR_INCLUDES@          \
	@PSTORE_INCLUDES@             \
	@LWADVAPI_INCLUDES@           \
	@LWMSG_INCLUDES@

liblwnetserverapi_la_SOURCES = \
       globals.c          \
       lwnet-server-cfg.c \
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
	@PSTORE_LIBS@         \
	@LWADVAPI_LIBS@       \
	@PTHREAD_LIBS@        \
	@LWMSG_LIBS@

liblwnetserverapi_la_LDFLAGS = \
        @LIBUNISTR_LDFLAGS@    \
        @PSTORE_LDFLAGS@       \
        @LWADVAPI_LDFLAGS@

noinst_HEADERS = \
		eventdlapi.h           \
		event_p.h              \
		evtstruct.h            \
		lwnet-cachedb.h        \
		lwnet-krb5_p.h         \
		lwnet_p.h              \
		lwnet-pstore_p.h       \
		lwnet-server-cfg_p.h   \
		state_p.h
