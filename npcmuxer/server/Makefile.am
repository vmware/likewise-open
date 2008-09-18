LIBCT_PREFIX=ctlib

sbin_PROGRAMS = npcmuxd
npcmuxd_CPPFLAGS = -I$(top_srcdir)/$(LIBCT_PREFIX)/include -I$(top_srcdir)/npclib/include -I$(top_srcdir)/include -D_GNU_SOURCE @SMBCLIENT_CPPFLAGS@ $(KRB5_CPPFLAGS)
npcmuxd_LDFLAGS = @SMBCLIENT_LDFLAGS@ @LIB_socket@ $(LTLDFLAGS)
npcmuxd_SOURCES = server.c proxy.c proxy_tcp.c proxy_np.c proxycreds.c
npcmuxd_LDADD = $(top_builddir)/$(LIBCT_PREFIX)/lib/libct.la $(KRB5_LDFLAGS) $(NETLOGON_LDFLAGS)
