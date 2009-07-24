lib_LTLIBRARIES = liblwmsg_nothr.la liblwmsg.la

AM_CFLAGS = @LWMSG_CFLAGS@
AM_CPPFLAGS = @LWMSG_CPPFLAGS@ -I$(top_srcdir)/include

liblwmsg_nothr_la_SOURCES = \
	assoc.c \
	assoc-marshal.c \
	assoc-call.c \
	call.c \
	convert.c \
	connection.c \
	connection-buffer.c \
	connection-state.c \
	connection-wire.c \
	connection-security.c \
	connection-marshal.c \
	message.c \
	protocol.c \
	data-context.c \
	data-graph.c \
	data-marshal.c \
	data-unmarshal.c \
	data-print.c \
	type.c \
	type-iterate.c \
	status.c \
	context.c \
	util.c \
	time.c \
	xnet.c \
	security.c \
	mt19937ar.c \
	session.c \
	session-default.c \
	archive.c \
	archive-format.c

liblwmsg_nothr_la_LIBADD = @LIB_XNET@ @LIB_DL@ @LIB_ICONV@
liblwmsg_nothr_la_LDFLAGS = $(AM_LDFLAGS) -export-symbols $(srcdir)/liblwmsg_nothr.sym

liblwmsg_la_SOURCES = \
	server.c \
	server-task.c \
	server-io.c \
	server-dispatch.c \
	server-call.c \
	client.c \
	session-shared.c

liblwmsg_la_LIBADD = liblwmsg_nothr.la @LIB_PTHREAD@
liblwmsg_la_LDFLAGS = $(AM_LDFLAGS) -export-symbols $(srcdir)/liblwmsg.sym
