provider_LTLIBRARIES = liblsass_auth_provider_local.la

liblsass_auth_provider_local_la_SOURCES = \
    lpaccess.c    \
    lpauthex.c    \
    lpcfg.c       \
    lpdomain.c    \
    lpenumstate.c \
    lpevent.c     \
    lpglobals.c   \
    lpgroup.c     \
    lpmain.c      \
    lpmarshal.c   \
    lpmisc.c      \
    lpobject.c    \
    lpuser.c

liblsass_auth_provider_local_la_CPPFLAGS = \
    -DLW_ENABLE_THREADS=1              \
    -I@top_srcdir@/server/include

#netapi is needed for NTLMv1EncryptChallenge
liblsass_auth_provider_local_la_LIBADD = \
    @top_builddir@/server/store/dsapi/libdsapi.la \
    @top_builddir@/utils/liblsautils.la \
    @top_builddir@/logging_r/liblsalog_r.la \
    @top_builddir@/server/srvutils/liblsasrvutils.la \
    @top_builddir@/lsaunistr/liblsaunistr.la \
    @top_builddir@/provider/local-provider/ipc/liblsalocalipc.la \
    @NETAPI_LIBS@

liblsass_auth_provider_local_la_LDFLAGS = \
	-module -avoid-version -export-dynamic -export-symbols=lp.sym \
	@NETAPI_LDFLAGS@
