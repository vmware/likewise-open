AM_CPPFLAGS = $(LWIO_FUSE_CPPFLAGS) -I$(top_srcdir)/include
AM_CFLAGS = $(LWIO_FUSE_CFLAGS)
AM_LDFLAGS = $(LWIO_FUSE_LDFLAGS)

bin_PROGRAMS = lwio-fuse-mount

lwio_fuse_mount_SOURCES = \
	common.c          \
	init.c            \
	getattr.c         \
	statfs.c          \
	readdir.c         \
	open.c            \
	release.c         \
	read.c            \
	write.c           \
	truncate.c        \
	mknod.c           \
	create.c          \
	unlink.c          \
	mkdir.c           \
	rmdir.c           \
	rename.c          \
	chmod.c           \
	chown.c           \
	entrypoint.c      \
	main.c

lwio_fuse_mount_LDADD =   \
	$(LIB_LWBASE)     \
	$(LIB_LWIOCLIENT) \
	$(LIB_LSACLIENT)  \
	$(LIB_FUSE)

