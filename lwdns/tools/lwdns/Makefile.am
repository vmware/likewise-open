bin_PROGRAMS = lw-update-dns

lw_update_dns_SOURCES = \
    globals.c   \
    lwkrb5.c    \
    main.c
    
lw_update_dns_CPPFLAGS = \
    @NETLOGON_INCLUDES@

lw_update_dns_LDADD = \
    @top_builddir@/lwdns/liblwdns.la \
    @NETLOGON_LIBS@
    
lw_update_dns_LDFLAGS = \
	@NETLOGON_LDFLAGS@
