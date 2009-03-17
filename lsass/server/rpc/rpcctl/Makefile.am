srv_srcdir = $(top_srcdir)/server
rpc_srcdir = $(srv_srcdir)/rpc

lib_LTLIBRARIES = librpcctl.la

librpcctl_la_SOURCES = \
    globals.c \
    svc_register.c \
    svc_unregister.c \
    svc_listen.c

LSASS_SRV_INCLUDES = -I$(srv_srcdir)/include
LSASS_INCLUDES = -I$(top_srcdir)/include

librpcctl_la_CFLAGS = \
    $(LSASS_SRV_INCLUDES) \
    $(LSASS_INCLUDES) \
    @DCERPC_INCLUDES@ \
    @LIBSECDESC_INCLUDES@

librpcctl_la_LIBADD = \
    @DCERPC_LIBS@ \
    @LIBSECDESC_LIBS@
