pamlib_LTLIBRARIES = pam_lsass.la

pam_lsass_la_SOURCES = \
       pam-acct.c      \
       pam-auth.c      \
       pam-context.c   \
       pam-conv.c      \
       pam-error.c     \
       pam-globals.c   \
       pam-passwd.c    \
       pam-session.c   \
       pam-notify.c

pam_lsass_la_CPPFLAGS = \
    -I@srcdir@ \
    -I@srcdir@/../include

pam_lsass_la_CFLAGS = -shared $(AM_CFLAGS)

pam_lsass_la_LIBADD = \
    @builddir@/../common/liblsaauth.la \
    @PAM_LIBS@

pam_lsass_la_LDFLAGS = -shared -module -avoid-version -export-dynamic
