libsecdesc_srcdir=$(top_srcdir)
SECDESC_LIBDIR=$(libsecdesc_srcdir)/src
SECDESC_INCDIR=$(libsecdesc_srcdir)/include

noinst_PROGRAMS=testsid

testsid_SOURCES=params.c test.c test_sid.c
testsid_CFLAGS=-g -I$(SECDESC_INCDIR)
testsid_LDFLAGS=-L$(SECDESC_LIBDIR)
testsid_LDADD=-lsecdesc
