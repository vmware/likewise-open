bin_PROGRAMS = lwio-copy

lwio_copy_SOURCES = \
    main.c \
    lwiocopy.c

lwio_copy_CPPFLAGS =  \
    @KRB5_INCLUDES@   \
    @NETLOGON_INCLUDES@ \
    -I@top_srcdir@         \
    -I@top_srcdir@/include

lwio_copy_LDADD = \
    @top_builddir@/client/liblwioclient.la\
    @KRB5_LIBS@ \
    @NETLOGON_LIBS@

lwio_copy_LDFLAGS =  \
    @KRB5_LDFLAGS@  \
    @NETLOGON_LDFLAGS@
