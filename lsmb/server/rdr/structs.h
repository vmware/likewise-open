#ifndef __STRUCTS_H__
#define __STRUCTS_H__

typedef struct _SMB_CLIENT_FILE_HANDLE
{
    pthread_mutex_t     mutex;

    PSTR      pszPrincipal;
    PSTR      pszCachePath;

    PSMB_TREE pTree;

    uint16_t  fid;

    uint64_t  llOffset;

} SMB_CLIENT_FILE_HANDLE, *PSMB_CLIENT_FILE_HANDLE;

#endif

