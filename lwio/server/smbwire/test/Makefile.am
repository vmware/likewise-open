noinst_LTLIBRARIES = libsmbwiremessages.la

SOURCE_FILES = \
	echo.c \
	negotiate.c \
	session_setup.c \
	tree_connect.c \
	open.c \
	close.c \
	read.c \
	write.c \
	transaction.c

AM_CPPFLAGS += -I@top_srcdir@/include

libsmbwiremessages_la_SOURCES = $(SOURCE_FILES)
libsmbwiremessages_la_LIBADD = transaction/namedpipe/libsmbwirenamedpipe.la \
	@LIBUNISTR_LIBS@
libsmbwiremessages_la_CPPFLAGS = -I@top_srcdir@ -I@top_srcdir@/include -DAPI
libsmbwiremessages_la_LDFLAGS = -static

if HAVE_MOONUNIT
noinst_LTLIBRARIES += smbwiremessages_mu.la
smbwiremessages_mu_la_SOURCES = $(SOURCE_FILES) test_stub.c
smbwiremessages_mu_la_LIBADD = libsmbwiremessages.la ../../test/libsmbtest.la \
	$(LIB_moonunit)
smbwiremessages_mu_la_LDFLAGS = -module -avoid-version -export-dynamic
smbwiremessages_mu_la_CPPFLAGS = -I@top_srcdir@ -I@top_srcdir@/include -DTEST \
	-DMU_LINK_STYLE=MU_LINK_UNDEFINED

BUILT_SOURCES = test_stub.c

test_stub.c: $(SOURCE_FILES)
	moonunit-stub -o $@ CPP="$(CPP)" CPPFLAGS="$(CPPFLAGS) \
		$(AM_CPPFLAGS) -DTEST -I@top_srcdir@ -I@top_srcdir@/include" $^
endif

wiremessagesincludedir=$(includedir)/smbwire

wiremessagesinclude_HEADERS = \
    negotiate.h       \
    session_setup.h   \
    tree_connect.h    \
    open.h	      \
    read.h	      \
    write.h	      \
    transaction.h

