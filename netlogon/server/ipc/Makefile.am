
noinst_LTLIBRARIES = liblwnetserveripc.la

liblwnetserveripc_la_CPPFLAGS = \
	-I@top_srcdir@/server/include
	
liblwnetserveripc_la_LIBADD = \
	@top_builddir@/ipc/liblwnetipc.la     \
	@top_builddir@/utils/liblwnetutils.la \
	@SOCKET_LIBS@                         \
    @NSL_LIBS@

liblwnetserveripc_la_SOURCES = \
       globals.c       \
       ipc_connect.c   \
       ipc_dcinfo.c    \
       ipc_state.c     \
       msg_dispatch.c

noinst_HEADERS = \
    includes.h        \
    externs_p.h       \
    ipc_connect_p.h   \
    ipc_dcinfo_p.h    \
    ipc_state_p.h     \
    msg_dispatch_p.h

