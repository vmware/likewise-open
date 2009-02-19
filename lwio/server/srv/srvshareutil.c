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

    SMBFreeMemory(ppShareInfoList);
}

VOID
SrvFreeShareInfo(
    DWORD       dwShareInfoLevel,
    PSHARE_INFO pShareInfo
    )
{
    SMB_SAFE_FREE_STRING(pShareInfo->pszName);
    SMB_SAFE_FREE_STRING(pShareInfo->pszPath);
    SMB_SAFE_FREE_STRING(pShareInfo->pszComment);
    SMB_SAFE_FREE_STRING(pShareInfo->pszSID);

    SMBFreeMemory(pShareInfo);
}


