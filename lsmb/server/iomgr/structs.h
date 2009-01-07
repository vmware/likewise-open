#ifndef __STRUCTS_H__
#define __STRUCTS_H__

typedef struct __NTVFS_PROVIDER
{
    PSTR pszProviderLibpath;
    PSTR pszId;

    PVOID pLibHandle;

    // Provider owns this
    PSTR pszName;
    // Provider owns this
    PNTVFS_DRIVER pFnTable;

    PFNVFSSHUTDOWNPROVIDER pFnShutdown;

} NTVFS_PROVIDER, *PNTVFS_PROVIDER;

struct SMB_FILE_HANDLE
{
    PNTVFS_PROVIDER pProvider;

    HANDLE hFile;
};

#endif

