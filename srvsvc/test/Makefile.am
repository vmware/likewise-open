
bin_PROGRAMS=testsrvsvc

testsrvsvc_SOURCES=TestSrvSvc.c Params.c SrvSvc.c

SRVSVC_INCDIR=$(top_srcdir)/Include
SRVSVC_INCLUDES=-I$(SRVSVC_INCDIR) -I$(top_srcdir)

testsrvsvc_CFLAGS=-g $(SRVSVC_INCLUDES) -D_GNU_SOURCE -DSRVSVC_BUILD
testsrvsvc_LDFLAGS= -rpath ${libdir}
testsrvsvc_LDADD=-ldcerpc -lunistr $(top_srcdir)/client/libsrvsvc.la
