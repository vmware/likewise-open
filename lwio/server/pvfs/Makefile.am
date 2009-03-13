
lib_LTLIBRARIES = libpvfs.sys.la

libpvfs_sys_la_SOURCES = \
    driver.c \
    create.c \
    read.c   \
    write.c  \
    flush.c \
    lock.c \
    deviceio.c \
    fsctrl.c \
    queryfile.c \
    querydir.c \
    queryvol.c \
    querysecdesc.c \
    close.c \
    create_file.c \
    create_dir.c \
    alloc.c \
    ccb.c \
    synchronize.c \
    errno.c \
    syswrap.c \
    util_open.c \
    util_dir.c \
    unixpath.c \
    string.c \
    wildcard.c \
    acl.c \
    acl_xattr.c \
    attrib.c \
    attrib_xattr.c \
    util_security.c \
    util_time.c \
    fileBasicInfo.c \
    fileStandardInfo.c \
    fileInternalInfo.c \
    fileBothDirInfo.c \
    fileEaInfo.c \
    fileStreamInfo.c \
    fileEndOfFileInfo.c \
    fileDispositionInfo.c \
    fileRenameInfo.c \
    fileFsAttribInfo.c \
    fileFsVolInfo.c \
    fileFsSizeInfo.c


libpvfs_sys_la_CPPFLAGS = \
    -I@top_srcdir@         \
    -I@top_srcdir@/include \
    -I@top_srcdir@/server/include \
    -D_FILE_OFFSET_BITS=64

libpvfs_sys_la_LIBADD = \
    @top_builddir@/utils/liblwioutils.la \
    @top_builddir@/server/iomgr2/libiomgr2.la \
    @UUID_LIBS@ \
    @PTHREAD_LIBS@ \
    @XATTR_LIBS@

libpvfs_sys_la_LDFLAGS = \
    -module -avoid-version -export-dynamic

