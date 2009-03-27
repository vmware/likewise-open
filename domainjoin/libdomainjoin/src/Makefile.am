ACLOCAL_AMFLAGS = -I m4

AM_CFLAGS = $(CPPFLAGS) -Wall -Werror-implicit-function-declaration

AM_CFLAGS += -I$(top_srcdir)/include  \
          -I$(srcdir)/../include \
          @CENTUTILS_INCLUDE@    \
          @LIBXML2_INCLUDE@      \
          @NETLOGON_INCLUDE@     \
          @LWDNS_INCLUDE@        \
          @EVENTLOG_INCLUDES@     \
          $(LWI_OS_FLAGS)

SCRIPTDIR = $(bindir)
AM_CPPFLAGS = -DSCRIPTDIR=\"$(SCRIPTDIR)\" -DLIBDIR=\"$(libdir)\" -DSYSCONFDIR=\"$(sysconfdir)\" -DDYNLIBEXT=\"$(DYNLIBEXT)\" $(LWI_OS_FLAGS)

lib_LTLIBRARIES = libdomainjoin.la

libdomainjoin_la_LIBADD = \
	$(CENTUTILS_LDFLAGS) \
	$(CENTUTILS_LIBS) \
	$(NETLOGON_LDFLAGS) \
	$(NETLOGON_LIBS) \
	$(LIBXML2_LDFLAGS) \
	$(LIBXML2_LIBS) \
	$(LWDNS_LIBS)   \
	$(EVENTLOG_LDFLAGS) \
	$(EVENTLOG_LIBS) \
        @LIB_XNET@

libdomainjoin_la_SOURCES = \
       djauthinfo.c   \
       djauthconf.c   \
       djddns.c       \
       djhostinfo.c   \
       djkrb5conf.c   \
       djlogger.c     \
       djlogincfg.c   \
       djmethodcfg.c  \
       djpamconf.c    \
       djdistroinfo.c \
       djsshconf.c    \
       djstr.c    \
       djnsswitch.c    \
       djparsehosts.c \
       djprocutils.c  \
       djroutines.c   \
       djsecuser.c    \
       djfirewall.c    \
       djtimemgmt.c \
       djmodule.c \
       djdaemonmgr.c \
       djsystemscripts.c \
       djaixparser.c \
       djconfig_mac.c \
       djapi.c

if BUILD_MACCODE
EXTRA_libdomainjoin_la_SOURCES = djdaemonmgr_nonmac.c
libdomainjoin_la_SOURCES += djdaemonmgr_mac.c
else
EXTRA_libdomainjoin_la_SOURCES = djdaemonmgr_mac.c
libdomainjoin_la_SOURCES += djdaemonmgr_nonmac.c
endif
