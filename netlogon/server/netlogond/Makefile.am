sbin_PROGRAMS = netlogond

netlogond_CPPFLAGS = \
	-I@top_srcdir@/server/include

netlogond_LDADD = \
	@top_builddir@/server/ipc/liblwnetserveripc.la \
	@top_builddir@/server/api/liblwnetserverapi.la \
	@top_builddir@/ipc/liblwnetipc.la \
	@top_builddir@/utils/liblwnetutils.la \
	@RT_LIBS@

netlogond_SOURCES = \
	globals.c    \
	listener.c   \
	main.c       \
	sighandler.c
