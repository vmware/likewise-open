noinst_LTLIBRARIES = liblwutilnetfile.la

liblwutilnetfile_la_SOURCES = \
    netfile.c

liblwutilnetfile_la_CPPFLAGS = \
    -I$(top_srcdir)/include \
    @LWNETAPI_INCLUDES@ \
    @LWADVAPI_INCLUDES@ \
    $(AM_CPPFLAGS)

liblwutilnetfile_la_LIBADD = -llwnetapi
