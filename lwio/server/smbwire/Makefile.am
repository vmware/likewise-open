lib_LTLIBRARIES = liblwiosmbwire.la

liblwiosmbwire_la_SOURCES = \
    wire_close.c         \
    wire_create_file.c   \
    wire_directory.c     \
    wire_echo.c          \
    wire_error.c         \
    wire_find_close2.c   \
    wire_lockx.c         \
    wire_negotiate.c     \
    packet.c             \
    wire_readx.c         \
    wire_session_setup.c \
    wire_transaction.c   \
    wire_tree_connect.c  \
    wire_writex.c

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
