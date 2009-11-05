bin_PROGRAMS = conf2reg

conf2reg_SOURCES = \
    conf2reg.c

conf2reg_CPPFLAGS = \
    $(AM_CPPFLAGS) \
    -I$(top_builddir)/include

conf2reg_LDADD = \
    $(top_builddir)/tools/conf2reg/agents/eventlog/libconfeventlog.la \
    $(top_builddir)/tools/conf2reg/agents/lsass/libconflsass.la \
    $(top_builddir)/tools/conf2reg/agents/lwiauth/libconflwiauth.la \
    $(top_builddir)/tools/conf2reg/agents/lwio/libconflwio.la \
    $(top_builddir)/tools/conf2reg/agents/netlogon/libconfnetlogon.la \
    $(top_builddir)/tools/conf2reg/agents/testparse/libconftestparse.la \
    $(top_builddir)/tools/conf2reg/agents/testsambaparse/libconftestsambaparse.la \
    $(top_builddir)/utils/libupgradeutils.la \
    @TDB_LIBS@



