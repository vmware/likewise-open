pamlib_LTLIBRARIES = pam_lsass.la

pam_lsass_la_SOURCES = \
       pam-acct.c      \
       pam-auth.c      \
       pam-config.c    \
       pam-context.c   \
       pam-conv.c      \
       pam-error.c     \
       pam-globals.c   \
       pam-logging.c   \
       pam-passwd.c    \
       pam-session.c

pam_lsass_la_CPPFLAGS = -I@srcdir@
pam_lsass_la_CFLAGS = -shared
pam_lsass_la_LIBADD = @top_builddir@/client/liblsaclient.la
pam_lsass_la_LDFLAGS = -shared -module -avoid-version -export-dynamic
