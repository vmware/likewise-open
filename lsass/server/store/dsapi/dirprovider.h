#ifndef __DIRPROVIDER_H__
#define __DIRPROVIDER_H__

DWORD
DirectoryGetProvider(
    PDIRECTORY_PROVIDER* ppProvider
    );

DWORD
DirectoryGetProviderInfo(
    PDIRECTORY_PROVIDER_INFO* ppProviderInfo
    );

DWORD
DirectoryLoadProvider(
    PDIRECTORY_PROVIDER_INFO pProviderInfo,
    PDIRECTORY_PROVIDER* ppProvider
    );

DWORD
DirectoryValidateProvider(
    PDIRECTORY_PROVIDER pProvider
    );

VOID
DirectoryReleaseProvider(
    PDIRECTORY_PROVIDER pProvider
    );

VOID
DirectoryFreeProvider(
    PDIRECTORY_PROVIDER pProvider
    );

VOID
DirectoryFreeProviderInfo(
    PDIRECTORY_PROVIDER_INFO pProviderInfo
    );

#endif /* __DIRPROVIDER_H__ */
