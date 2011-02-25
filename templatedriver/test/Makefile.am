AM_CPPFLAGS = -I$(top_srcdir)/include

noinst_PROGRAMS = testtemplatedriver

testtemplatedriver_SOURCES =   \
    test.c

TEMPLATEDRIVER_INCLUDES =      \
    -I$(TEMPLATEDRIVER_INCDIR) \
    -I$(top_srcdir) 

testtemplatedriver_CFLAGS =    \
    $(AM_CPPFLAGS)             \
    $(TEMPLATEDRIVER_INCLUDES) \
    -D_GNU_SOURCE

testtemplatedriver_LDFLAGS =   \
    -rpath ${libdir} 

#testtemplatedriver_LDADD =     \
#    $(top_builddir)/client/libtemplatedriver.la
