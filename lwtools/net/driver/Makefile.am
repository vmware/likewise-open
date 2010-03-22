bin_PROGRAMS = lwnet

lwnet_SOURCES = \
    main.c

lwnet_CPPFLAGS = \
    $(AM_CPPFLAGS) \
    -I$(top_builddir)/include

lwnet_LDADD = \
    $(top_builddir)/net/share/liblwutilnetshare.la \
    $(top_builddir)/net/common/liblwnetutils.la \
    @LWNETAPI_LIBS@ \
    @LWADVAPI_LIBS@
    
