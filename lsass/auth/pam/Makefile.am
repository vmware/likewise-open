pamlib_LTLIBRARIES = pam_lsass.la

pam_lsass_la_SOURCES = \
       pam-acct.c      \
       pam-auth.c      \
       pam-context.c   \
       pam-conv.c      \
       pam-error.c     \
       pam-globals.c   \
       pam-passwd.c    \
       pam-session.c

pam_lsass_la_CPPFLAGS = \
    -I@srcdir@ \
    -I@top_srcdir@/auth/include

pam_lsass_la_CFLAGS = -shared

pam_lsass_la_LIBADD = \
    @top_builddir@/auth/common/liblsaauth.la \
    @PAM_LIBS@

pam_lsass_la_LDFLAGS = -shared -module -avoid-version -export-dynamic
