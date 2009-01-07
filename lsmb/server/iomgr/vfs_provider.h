#ifndef __VFS_PROVIDER_H__
#define __VFS_PROVIDER_H__

DWORD
IOMgrInitProviders(
    PCSTR pszConfigFilePath
    );

VOID
IOMgrFreeProviders(
    VOID
    );

#endif /* __VFS_PROVIDER_H__ */
