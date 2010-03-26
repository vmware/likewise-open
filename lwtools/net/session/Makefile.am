noinst_LTLIBRARIES = liblwutilnetsession.la

liblwutilnetsession_la_SOURCES = \
    netsession.c

liblwutilnetsession_la_CPPFLAGS = \
    -I$(top_srcdir)/include \
    @LWNETAPI_INCLUDES@ \
    @LWADVAPI_INCLUDES@ \
    $(AM_CPPFLAGS)

liblwutilnetsession_la_LIBADD = -llwnetapi
