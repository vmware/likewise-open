compat_subdirs = \
	auth \
	nsswitch

full_subdirs= \
	$(compat_subdirs) \
	lwmapsecurity \
	wbclient \
	lwicompat \
	access

if ENABLE_COMPAT
SUBDIRS = $(compat_subdirs)
else
SUBDIRS = $(full_subdirs)
endif
