SUBDIRS = include

lib_LTLIBRARIES = liblwrpcrt.la

liblwrpcrt_la_SOURCES = lwrpcrt.c

liblwrpcrt_la_CPPFLAGS = $(AM_CPPFLAGS) \
			$(DCERPC_INCLUDES)

liblwrpcrt_la_LDFLAGS = $(AM_LDFLAGS) \
			$(DCERPC_LDFLAGS)

liblwrpcrt_la_LIBADD = \
			$(DCERPC_LIBS)
