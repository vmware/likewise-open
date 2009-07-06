noinst_LTLIBRARIES = liblsaauth.la

liblsaauth_la_SOURCES = \
    globals.c    \
    libmain.c    \
    pam-config.c \
    pam-logging.c

liblsaauth_la_CPPFLAGS = \
    -I@srcdir@ \
    -I@srcdir@/../include

liblsaauth_la_LIBADD = @top_builddir@/client/liblsaclient.la

