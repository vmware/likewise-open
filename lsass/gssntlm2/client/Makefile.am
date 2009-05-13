lib_LTLIBRARIES = libgssntlm.la

libgssntlm_la_SOURCES = \
       gsscontext.c \
       protomsg.c \
       credential.c \
       crypt.c \
       seal.c \
       sign.c \
       gssutil.c \
       translate.c

libgssntlm_la_CPPFLAGS = \
	-I@top_srcdir@/gssntlm/include

libgssntlm_la_LIBADD = \
	@top_builddir@/gssntlm/common/libntlmcommon.la \
	@top_builddir@/client/liblsaclient.la \
	@top_builddir@/lsaunistr/liblsaunistr.la

