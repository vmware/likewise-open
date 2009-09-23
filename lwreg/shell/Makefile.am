noinst_PROGRAMS = regshell

bin_PROGRAMS = regshell

#regshell
regshell_SOURCES = \
    cmdparse.c \
    regshell.c

regshell_LDFLAGS = -static


regshell_LDADD = \
        @top_builddir@/shellutil/librsutils.la \
        @top_builddir@/utils/libregutils.la \
        @top_builddir@/parse/libregparse.la \
        @top_builddir@/client/libregclient.la \
        @top_builddir@/libedit-20090905-3.0/src/libedit.la \
        @LWADVAPI_NOTHR_LIBS@ \
        @LWMSG_NOTHR_LIBS@

regshell_CPPFLAGS = \
    $(AM_CFLAGS)   \
    -I@top_srcdir@/shellutil \
    -I@top_srcdir@/server/include \
    -I@top_srcdir@/libedit-20090905-3.0/src \
    @LWBASE_INCLUDES@
