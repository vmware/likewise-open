lib_LTLIBRARIES = liblwpsapi.la

liblwpsapi_la_SOURCES =  \
	libmain.c        \
	lwps-provider.c

liblwpsapi_la_LIBADD = @top_builddir@/utils/liblwps-utils.la @DL_LIBS@

noinst_HEADERS = \
	lwps-provider_p.h