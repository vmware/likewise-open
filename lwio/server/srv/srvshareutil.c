#include "includes.h"

VOID
SrvFreeShareInfoList(
    DWORD        dwShareInfoLevel,
    PSHARE_INFO* ppShareInfoList,
    DWORD        dwNumShares
    )
{
    DWORD iShare = 0;

    for (; iShare < dwNumShares; iShare++)
    {
        PSHARE_INFO pShareInfo = *(ppShareInfoList + iShare);

        if (pShareInfo)
        {
            SrvFreeShareInfo(dwShareInfoLevel, pShareInfo);
        }
    }

    LwRtlMemoryFree(ppShareInfoList);
}

VOID
SrvFreeShareInfo(
    DWORD       dwShareInfoLevel,
    PSHARE_INFO pShareInfo
    )
{
    LWIO_SAFE_FREE_STRING(pShareInfo->pszName);
    LWIO_SAFE_FREE_STRING(pShareInfo->pszPath);
    LWIO_SAFE_FREE_STRING(pShareInfo->pszComment);
    LWIO_SAFE_FREE_STRING(pShareInfo->pszSID);

    LwRtlMemoryFree(pShareInfo);
}


