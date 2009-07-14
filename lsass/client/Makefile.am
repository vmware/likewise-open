SUBDIRS = \
    api \
    ad \
    local
#    ntlm

lib_LTLIBRARIES = liblsaclient.la

liblsaclient_la_SOURCES =

liblsaclient_la_LIBADD = \
    @builddir@/api/liblsaclient_api.la \
    @builddir@/ad/liblsaclient_ad.la \
    @builddir@/local/liblsaclient_local.la
#    @builddir@/ntlm/liblsaclient_ntlm.la
