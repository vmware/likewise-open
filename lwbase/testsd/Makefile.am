libsecdesc_srcdir = $(top_srcdir)
SECDESC_LIBDIR = $(libsecdesc_srcdir)/src
SECDESC_INCDIR = $(libsecdesc_srcdir)/include

noinst_PROGRAMS = testsd

testsd_SOURCES = \
    params.c \
    test.c \
    test_sid.c

testsd_CFLAGS = \
    -g -I$(SECDESC_INCDIR)

testsd_LDFLAGS = -L$(SECDESC_LIBDIR)
testsd_LDADD = -lsecdesc
