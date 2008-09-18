LIBCT_PREFIX=../../ctlib
LIBCT_INCDIR=$(LIBCT_PREFIX)/include

LIBNPC_PREFIX=../../npclib
LIBNPC_INCDIR=$(LIBNPC_PREFIX)/include
LIBNPC_LIBDIR=$(LIBNPC_PREFIX)/lib
LIBNPC_ARCHIVE=$(LIBNPC_LIBDIR)/libnpc.la

AM_CPPFLAGS=-I$(LIBCT_INCDIR) -I$(LIBNPC_INCDIR) -D_GNU_SOURCE

bin_PROGRAMS = npctest
npctest_SOURCES = npctest.c
npctest_LDADD = $(LIBNPC_ARCHIVE)
#npctest_LDFLAGS = -L/usr/lib -lkrb5 -lk5crypto -lcom_err -lkrb5support -lresolv
