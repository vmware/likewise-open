lib_LTLIBRARIES = liblwiosmbwire.la

liblwiosmbwire_la_SOURCES = \
    wire_close.c         \
    wire_echo.c          \
    wire_negotiate.c     \
    wire_open.c          \
    packet.c             \
    wire_read.c          \
    wire_session_setup.c \
    wire_transaction.c   \
    wire_tree_connect.c  \
    wire_write.c

liblwiosmbwire_la_CPPFLAGS =     \
    -DAPI                         \
    -I@top_srcdir@                \
    -I@top_srcdir@/include        \
    -I@top_srcdir@/server/include

liblwiosmbwire_la_LIBADD = \
	@top_builddir@/smbkrb5/liblwiosmbkrb5.la \
	@top_builddir@/utils/liblwioutils.la \
	@CRYPTO_LIBS@ \
	@SEM_LIBS@
