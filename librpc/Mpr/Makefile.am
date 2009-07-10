librpc_srcdir=$(top_srcdir)
liberror_srcdir=$(top_srcdir)/Error

noinst_LTLIBRARIES = libmpr.la

libmpr_la_SOURCES = \
	WNetAddConnection2.c WNetCancelConnection2.c

libmpr_la_LIBADD = -lunistr

libmpr_la_LDFLAGS = $(UNISTR_LDFLAGS)

libmpr_la_CFLAGS = $(AM_CFLAGS)

INCLUDES = $(UNISTR_INCLUDES) \
	   -I$(librpc_srcdir)/include -I$(liberror_srcdir)
