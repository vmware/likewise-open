ACLOCAL_AMFLAGS = -I m4

compat_subdirs = \
	common \
        client \
	logging_r \
	interop

full_subdirs= \
	$(compat_subdirs) \
	lsaunistr         \
	gssntlm	          \
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
