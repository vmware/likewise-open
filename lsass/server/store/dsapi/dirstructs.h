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

typedef struct _DIRECTORY_PROVIDER
{
    LONG refCount;

    PSTR                   pszProviderName;
    PVOID                  pLibHandle;
    PFNSHUTDOWNDIRPROVIDER pfnShutdown;
    PDIRECTORY_PROVIDER_FUNCTION_TABLE pProviderFnTbl;

    PDIRECTORY_PROVIDER_INFO pProviderInfo;

} DIRECTORY_PROVIDER, *PDIRECTORY_PROVIDER;

typedef struct _DIRECTORY_GLOBALS
{
    pthread_mutex_t mutex;

    PDIRECTORY_PROVIDER pProvider;

} DIRECTORY_GLOBALS, *PDIRECTORY_GLOBALS;

#endif /* __DIRSTRUCTS_H__ */
