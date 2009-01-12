lib_LTLIBRARIES = libsmb.la

libsmb_la_SOURCES = \
    close.c         \
    echo.c          \
    negotiate.c     \
    open.c          \
    read.c          \
    session_setup.c \
    transaction.c   \
    tree_connect.c  \
    write.c

libsmb_la_CPPFLAGS =     \
    -DAPI                         \
    -I@top_srcdir@                \
    -I@top_srcdir@/include        \
    -I@top_srcdir@/server/include

libsmb_la_LIBADD = \
	@top_builddir@/smbkrb5/libsmbkrb5.la \
	@top_builddir@/utils/libsmbutils.la \
	@CRYPTO_LIBS@ \
	@SEM_LIBS@
