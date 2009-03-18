#ifndef __DIRPROVIDER_H__
#define __DIRPROVIDER_H__

DWORD
DirectoryGetProviderInfo(
    PDIRECTORY_PROVIDER_INFO* ppProviderInfo
    );

VOID
DirectoryFreeProviderInfo(
    PDIRECTORY_PROVIDER_INFO pProviderInfo
    );

#endif /* __DIRPROVIDER_H__ */
