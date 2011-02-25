if ENABLE_STATIC_DRIVERS
noinst_LTLIBRARIES = libtemplatedriver.sys.la
DRIVER_CFLAGS =
DRIVER_LDFLAGS =
else
lib_LTLIBRARIES = libtemplatedriver.sys.la
DRIVER_CFLAGS = -shared
DRIVER_LDFLAGS = -module -shared -avoid-version
endif

libtemplatedriver_sys_la_SOURCES = \
	dispatch.c		\
    driver.c		\
	globals.c		\
	initialize.c

libtemplatedriver_sys_la_CPPFLAGS = \
    -DLW_ENABLE_THREADS=1  \
    -I@top_srcdir@         \
    -I@top_srcdir@/include \
    $(LWIO_INCLUDES)

libtemplatedriver_sys_la_CFLAGS = $(AM_CFLAGS) $(DRIVER_CFLAGS)

libtemplatedriver_sys_la_LIBADD = \
    $(LWIO_LIBS)    \
    @PTHREAD_LIBS@

#    @top_builddir@/server/iomgr/libiomgr.la 
#    @top_builddir@/libshareinfo/liblwioshareinfo.la 

libtemplatedriver_sys_la_LDFLAGS = \
    $(DRIVER_LDFLAGS)   \
    $(LWIO_LDFLAGS)

