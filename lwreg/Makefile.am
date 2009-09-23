ACLOCAL_AMFLAGS = -I m4

compat_subdirs = \
    utils        \
    include      \
    ipc          \
    client       
    

full_subdirs=         \
    $(compat_subdirs) \
    logging_r         \
    parse             \
    server            \
    shellutil         \
    test              \
    tools             \
    libedit-20090905-3.0 \
    shell

if ENABLE_COMPAT
SUBDIRS = $(compat_subdirs)
else
SUBDIRS = $(full_subdirs)
endif

