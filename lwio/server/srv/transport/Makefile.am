
if HAVE_SELECT

METHOD=lwselect

endif

if HAVE_POLL

METHOD=lwpoll

endif

if HAVE_KQUEUE

METHOD=lwkqueue

endif

if HAVE_EPOLL

METHOD=lwepoll

endif

SUBDIRS = \
    $(METHOD) \
    api

