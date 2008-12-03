#ifndef __FSERV_H__
#define __FSERV_H__

/* Opaque handle to a server */
typedef struct FServ FServ;

/* Opqaue handle to a file */
typedef struct FServFile FServFile;

typedef enum FServMode
{
    FSERV_MODE_READ = 1,
    FSERV_MODE_WRITE = 2,
    FSERV_MODE_APPEND = 4
} FServMode;

/* Connect to local fserv */
int
fserv_connect(
    FServ** out_fserv
    );

/* Disconnect an fserv connection */
int
fserv_disconnect(
    FServ* fserv
    );

/* Open a file using an fserv connection */
int
fserv_open(
    FServ* fserv,
    const char* path,
    FServMode mode,
    FServFile** out_file
    );

/* Read from a file */
int
fserv_read(
    FServFile* file,
    unsigned long size,
    void* buffer,
    unsigned long* size_read
    );

/* Write to a file */
int
fserv_write(
    FServFile* file,
    unsigned long size,
    void* buffer
    );

/* Close a file */
int
fserv_close(
    FServFile* file
    );

#endif
