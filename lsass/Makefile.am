ACLOCAL_AMFLAGS = -I m4

compat_subdirs = \
	utils \
	logging_r \
        ipc            \
        ntlmipc \
        client         \
        provider       \
	interop

full_subdirs= \
	$(compat_subdirs) \
	lsaunistr         \
	gssntlm	          \
	ntlmclient \
        join                \
        server            \
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
