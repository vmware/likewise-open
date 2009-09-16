noinst_PROGRAMS = regshell

bin_PROGRAMS = regshell

#regshell
regshell_SOURCES = \
    cmdparse.c \
    regshell.c \
    import.c

regshell_LDFLAGS = -static


regshell_LDADD = \
        @top_builddir@/utils/libregutils.la \
        @top_builddir@/parse/libregparse.la \
        @top_builddir@/client/libregclient.la \
        @LWADVAPI_NOTHR_LIBS@ \
        @LWMSG_NOTHR_LIBS@

regshell_CPPFLAGS = \
    $(AM_CFLAGS)   \
    -I@top_srcdir@/server/include \
    @LWBASE_INCLUDES@
