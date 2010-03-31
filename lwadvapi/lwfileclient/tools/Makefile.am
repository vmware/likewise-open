noinst_PROGRAMS = lwfileclient

lwfileclient_SOURCES = testfileclient.c

lwfileclient_CPPFLAGS = \
        -I@top_srcdir@/api \
        -I@top_srcdir@/include \
		-I@LWIO_INCLUDE@

lwfileclient_LDADD = \
        @top_builddir@/api/liblwfileclient.la \
        @top_builddir@/utils/liblwfileclientutils.la \
        @LWIO_LIBS@

lwfileclient_LDFLAGS = \
        @LWIO_LDFLAGS@ \
        @KRB5_LDFLAGS@

