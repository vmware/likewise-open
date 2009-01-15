lib_LTLIBRARIES = libsecdesc.la

libsecdesc_la_SOURCES = sid.c \
                        sidmem.c \
                        sidstr.c \
                        polhnd.c

AM_CFLAGS += -D_GNU_SOURCE
AM_CFLAGS += @UNISTR_CFLAGS@ @UNISTR_INCLUDES@
AM_LDFLAGS += @UNISTR_LDFLAGS@ @UNISTR_LIBS@
AM_LDFLAGS += @UUID_LIBS@
