AM_CFLAGS = $(LWBASE_CFLAGS)
AM_CPPFLAGS = $(LWBASE_CPPFLAGS) -I$(top_srcdir)/include
AM_LDFLAGS = $(LWBASE_LDFLAGS)

TEST_SOURCES = test-security.c

lib_LTLIBRARIES = liblwbase_nothr.la liblwbase.la libsecdesc.la 

if HAVE_MOONUNIT
lib_LTLIBRARIES += lwbase_mu.la
endif

liblwbase_nothr_la_SOURCES = \
	errno.c \
	ntstatus.c \
        rtlmemory.c \
        rtlstring.c \
        rtlstring_cstring.c \
        rtlstring_wc16string.c \
        rtlstring_unicode.c \
        rtlstring_ansi.c \
        rbtree.c \
        security/security-sid.c \
        security/security-acl.c \
        security/security-sd.c \
        security/security-token.c

liblwbase_nothr_la_LIBADD = \
        @LIBUNISTR_LIBS@

liblwbase_la_SOURCES = \
	atomic.c

liblwbase_la_LIBADD = liblwbase_nothr.la

libsecdesc_la_SOURCES = \
        rtlsid.c \
        sid.c \
        sidstr.c \
        rtlpolhnd.c

libsecdesc_la_LIBADD = \
        @LIBUUID_LIBS@ \
        liblwbase_nothr.la

lwbase_mu_la_SOURCES = \
	test-stub.c \
	$(TEST_SOURCES)

lwbase_mu_la_LIBADD = \
	liblwbase.la  \
	$(LIB_MOONUNIT)

lwbase_mu_la_LDFLAGS = -module -avoid-version

test-stub.c: $(TEST_SOURCES)
	$(MOONUNIT_STUB) -o $@ CPP="$(CPP)" CPPFLAGS="$(CPPFLAGS) $(AM_CPPFLAGS)" $^