ACLOCAL_AMFLAGS = -I m4

compat_subdirs = \
	    utils      \
	    logging_r  \
        ipc            \
        client         \
        nsswitch       \
        pam @MACCLIENT_DIR@

full_subdirs= \
	$(compat_subdirs) \
	lsaunistr         \
        wbclient          \
	gssntlm	          \
        server            \
        tools             \
        etc               \
        include           \
        scripts           \
        tests          

if ENABLE_COMPAT
SUBDIRS = $(compat_subdirs)
else
SUBDIRS = $(full_subdirs)
endif

