#ifndef __DIRSTRUCTS_H__
#define __DIRSTRUCTS_H__

typedef struct _DIRECTORY_PROVIDER_INFO
{
    DirectoryType dirType;
    PSTR          pszProviderPath;
} DIRECTORY_PROVIDER_INFO, *PDIRECTORY_PROVIDER_INFO;

typedef struct _DIRECTORY_CONTEXT
{
    DirectoryType directoryType;
    HANDLE        hBindHandle;
} DIRECTORY_CONTEXT, *PDIRECTORY_CONTEXT;

#endif /* __DIRSTRUCTS_H__ */
