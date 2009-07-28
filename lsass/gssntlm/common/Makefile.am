AM_CPPFLAGS = @AM_CPPFLAGS@ -I$(top_srcdir)/include -I$(top_srcdir)/gssntlm/include

noinst_LTLIBRARIES = libntlmcommon.la

libntlmcommon_la_SOURCES = \
       debug.c      \
       authuser.c   \
       globals.c    \
       ntlmcrypt.c  \
       ntlmdlist.c  \
       ntlmencode.c \
       utils.c

libntlmcommon_la_LDFLAGS = @CRYPTO_LDFLAGS@
libntlmcommon_la_LIBADD = \
	@top_builddir@/common/liblsacommon.la \
	@top_builddir@/lsaunistr/liblsaunistr.la \
	@PTHREAD_LIBS@ @CRYPTO_LIBS@ @DL_LIBS@
