lib_LTLIBRARIES = liblwiosmb.la

liblwiosmb_la_SOURCES = \
    close.c         \
    echo.c          \
    negotiate.c     \
    open.c          \
    packet.c        \
    read.c          \
    session_setup.c \
    transaction.c   \
    tree_connect.c  \
    write.c

liblwiosmb_la_CPPFLAGS =     \
    -DAPI                         \
    -I@top_srcdir@                \
    -I@top_srcdir@/include        \
    -I@top_srcdir@/server/include

liblwiosmb_la_LIBADD = \
	@top_builddir@/smbkrb5/liblwiosmbkrb5.la \
	@top_builddir@/utils/liblwioutils.la \
	@CRYPTO_LIBS@ \
	@SEM_LIBS@
