ACLOCAL_AMFLAGS = -I m4

compat_subdirs = \
	    utils      \
	    logging_r  \
        ipc            \
        client         \
        provider       \
        auth @MACCLIENT_DIR@ \
        nsswitch

full_subdirs= \
	$(compat_subdirs) \
	lsaunistr         \
        wbclient          \
	lwicompat	  \
	gssntlm	          \
        server            \
        access            \
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

