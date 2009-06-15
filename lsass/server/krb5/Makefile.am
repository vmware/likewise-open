lib_LTLIBRARIES = liblsakrb5.la

liblsakrb5_la_SOURCES = \
       globals.c \
       krbtgt.c  \
       lwkrb5.c  \
       lwkrb5smb.c

liblsakrb5_la_CPPFLAGS = \
	-DLW_ENABLE_THREADS=1         \
	-I@top_srcdir@/server/include \
	@KRB5_INCLUDES@ \
	@KRB5PAC_INCLUDES@ \
	@LWIO_INCLUDES@

liblsakrb5_la_LIBADD = \
	@top_builddir@/utils/liblsautils.la \
	@top_builddir@/lsaunistr/liblsaunistr.la \
    @top_builddir@/logging_r/liblsalog_r.la \
	@KRB5_LIBS@ \
	@KRB5PAC_LIBS@ \
	@LWIO_LIBS@ \
	-llber

liblsakrb5_la_LDFLAGS = \
	@OPENLDAP_LDFLAGS@ \
	@KRB5_LDFLAGS@ \
	@KRB5PAC_LDFLAGS@ \
	@LWIO_LDFLAGS@
