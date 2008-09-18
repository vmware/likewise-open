librpc_srcdir=$(top_srcdir)
liberror_srcdir=$(top_srcdir)/Error

lib_LTLIBRARIES = libmpr.la

libmpr_la_SOURCES = \
	WNetAddConnection2.c WNetCancelConnection2.c

libmpr_la_LIBADD = -lnpc -lunistr $(top_builddir)/Error/liberror.la

libmpr_la_LDFLAGS = $(NPC_LDFLAGS) $(UNISTR_LDFLAGS)

INCLUDES = $(NPC_INCLUDES) $(UNISTR_INCLUDES) \
	   -I$(librpc_srcdir)/Include -I$(liberror_srcdir)