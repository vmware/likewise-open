bin_PROGRAMS = lw-export-dc-cache

lw_export_dc_cache_CPPFLAGS = \
	-I@top_srcdir@/server/include \
	-I@top_srcdir@/server/api
	
lw_export_dc_cache_LDADD = \
	@top_builddir@/server/api/liblwnetserverapi.la

lw_export_dc_cache_SOURCES = main.c
