lib_LTLIBRARIES = libktkrb5.la

libktkrb5_la_CFLAGS = \
       @LIBUNISTR_INCLUDES@ \
       @KRB5_INCLUDES@      \
       @OPENLDAP_INCLUDES@

libktkrb5_la_SOURCES = \
       globals.c    \
       krbtgt.c     \
       ktkrb5.c     \
       keytab.c     \
       keytab_w16.c \
       ldap.c       \
       ldap_w16.c

libktkrb5_la_LDFLAGS = \
       @LIBUNISTR_LDFLAGS@ \
       @KRB5_LDFLAGS@      \
       @OPENLDAP_LDFLAGS@

libktkrb5_la_LIBADD = \
       @top_builddir@/utils/libktutils.la \
       @OPENLDAP_LIBS@                    \
       @KRB5_LIBS@                        \
       @PTHREAD_LIBS@

