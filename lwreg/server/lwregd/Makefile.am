sbin_PROGRAMS = lwregd

lwregd_SOURCES = \
    globals.c       \
    listener.c      \
    main.c          \
    sighandler.c

lwregd_CPPFLAGS =              \
    -DLW_ENABLE_THREADS=1         \
    $(AM_CFLAGS)                  \
    -I@top_srcdir@/server/include \
    @LWADVAPI_INCLUDES@

lwregd_LDADD =                             \
    @top_builddir@/server/api/libregserverapi.la \
    @top_builddir@/client/libregclient.la     \
    @top_builddir@/utils/libregutils.la       \
    @top_builddir@/logging_r/libreglog_r.la   \
    @DS_FRAMEWORK_LDFLAGS@                    \
    @LWMSG_LIBS@
    
install-data-local:
	$(mkinstalldirs) $(DESTDIR)$(registrycachedir)
