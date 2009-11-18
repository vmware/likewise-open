bin_PROGRAMS = pstore2reg

pstore2reg_SOURCES = \
    pstore2reg.c \
    pstore-sqlite.c

pstore2reg_CPPFLAGS = \
    $(AM_CPPFLAGS) \
    -I$(top_builddir)/include \
    @SQLITE_INCLUDES@

pstore2reg_LDADD = \
    $(top_builddir)/utils/libupgradeutils.la \
    @SQLITE_LIBS@

