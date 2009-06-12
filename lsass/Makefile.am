ACLOCAL_AMFLAGS = -I m4

compat_subdirs = \
	    utils      \
	    logging_r  \
        ipc            \
        ntlmipc \
        client         \
        provider       \
        auth @MACCLIENT_DIR@ \
        nsswitch

full_subdirs= \
	$(compat_subdirs) \
	lwmapsecurity     \
	lsaunistr         \
        wbclient          \
	lwicompat	  \
	gssntlm	          \
	ntlmclient \
        server            \
        access            \
        tools             \
        etc               \
        include           \
        scripts           \
        tests             \
        docs

if ENABLE_COMPAT
SUBDIRS = $(compat_subdirs)
else
SUBDIRS = $(full_subdirs)
endif

