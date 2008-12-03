
noinst_LTLIBRARIES = liblwnetserveripc.la

liblwnetserveripc_la_CPPFLAGS = \
	-I@top_srcdir@/server/include

liblwnetserveripc_la_LIBADD = \
	@top_builddir@/ipc/liblwnetipc.la     \
	@top_builddir@/utils/liblwnetutils.la \
	@SOCKET_LIBS@                         \
	@NSL_LIBS@                            \
	-llwmsg

liblwnetserveripc_la_SOURCES = \
	ipc_dcinfo.c    \
	globals.c

noinst_HEADERS = \
    includes.h        \
    externs_p.h       \
    ipc_dcinfo_p.h

