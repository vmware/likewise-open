AM_CPPFLAGS += -I$(top_srcdir)/include

lib_LTLIBRARIES = liblwmapsecurity.la

liblwmapsecurity_la_SOURCES = \
	lwmapsecurity.c

liblwmapsecurity_la_LIBADD = \
        @LWBASE_LIBS@ \
        @LIB_PTHREAD@ \
        @DL_LIBS@
