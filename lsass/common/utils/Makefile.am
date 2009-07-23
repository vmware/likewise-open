noinst_LTLIBRARIES = liblsautils.la

liblsautils_la_SOURCES =       \
       consolelog.c            \
       dcinfo.c                \
       dlinkedlist.c           \
       domaininfo.c            \
       filelog.c               \
       fileutils.c             \
       globals.c               \
       lsacache.c              \
       lsacfg.c                \
       lsadns.c                \
       lsaerror.c              \
       lsagroupinfo.c          \
       lsaartefactinfo.c       \
       lsabitvector.c          \
       lsalogger.c             \
       lsamem.c                \
       lsapaths.c              \
       lsastack.c              \
       lsastr.c                \
       lsaprincipal.c          \
       lsahash.c               \
       lsastatus.c             \
       lsastrerror.c           \
       lsasecurityidentifier.c \
       lsauserinfo.c           \
       lsaloginfo.c            \
       nisnicknames.c          \
       sidinfo.c               \
       sysfuncs.c              \
       syslog.c                \
       datablob.c              \
       lsaxnet.c               \
       lsaauthex.c             \
       lsalist.c               \
       sid.c                   \
       lwsecurityidentifier.c

liblsautils_la_CPPFLAGS = \
	$(AM_CPPFLAGS) \
	$(LWBASE_INCLUDES) \
	$(LWADVAPI_INCLUDES)

liblsautils_la_LIBADD = \
	$(RESOLV_LIBS) \
	$(NSL_LIBS) \
	$(LWBASE_NOTHR_LIBS) \
	$(LWADVAPI_NOTHR_LIBS) \
	$(LWMSG_NOTHR_LIBS)
