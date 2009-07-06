noinst_LTLIBRARIES = libntlmcommon.la

libntlmcommon_la_SOURCES = \
       debug.c      \
       authuser.c   \
       globals.c    \
       ntlmcrypt.c  \
       ntlmdlist.c  \
       ntlmencode.c \
       utils.c

libntlmcommon_la_CPPFLAGS = -I@top_srcdir@/gssntlm/include
libntlmcommon_la_LDFLAGS = @CRYPTO_LDFLAGS@
libntlmcommon_la_LIBADD = \
	@top_builddir@/utils/liblsautils.la \
	@top_builddir@/lsaunistr/liblsaunistr.la \
	@PTHREAD_LIBS@ @CRYPTO_LIBS@ @DL_LIBS@
