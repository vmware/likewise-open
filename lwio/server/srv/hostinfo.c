#include "includes.h"

static
VOID
SrvFreeHostInfo(
    PSRV_HOST_INFO pHostinfo
    );

NTSTATUS
SrvAcquireHostInfo(
    PSRV_HOST_INFO  pOrigHostInfo,
    PSRV_HOST_INFO* ppNewHostInfo
    )
{
    NTSTATUS ntStatus = 0;
    CHAR szHostname[256];
    PSTR pszDomain = NULL;
    PSRV_HOST_INFO pHostInfo = NULL;
    PSRV_HOST_INFO pNewHostInfo = NULL;

    if (!pOrigHostInfo)
    {
        ntStatus = LW_RTL_ALLOCATE(
                        &pHostInfo,
                        SRV_HOST_INFO,
                        sizeof(SRV_HOST_INFO));
        BAIL_ON_NT_STATUS(ntStatus);

        pHostInfo->refcount = 1;

        pthread_rwlock_init(&pHostInfo->mutex, NULL);
        pHostInfo->pMutex = &pHostInfo->mutex;

        ntStatus = LWNetGetCurrentDomain(&pszDomain);
        BAIL_ON_NT_STATUS(ntStatus);

        if (gethostname(szHostname, sizeof(szHostname)) != 0)
        {
            ntStatus = LwUnixErrnoToNtStatus(errno);
            BAIL_ON_NT_STATUS(ntStatus);
        }

        ntStatus = SMBAllocateString(
                        szHostname,
                        &pHostInfo->pszHostname);
        BAIL_ON_NT_STATUS(ntStatus);

        ntStatus = SMBAllocateString(
                        pszDomain,
                        &pHostInfo->pszDomain);
        BAIL_ON_NT_STATUS(ntStatus);

        pNewHostInfo = pHostInfo;
    }
    else
    {
        pNewHostInfo = pOrigHostInfo;
        InterlockedIncrement(&pNewHostInfo->refcount);
    }

    *ppNewHostInfo = pNewHostInfo;

cleanup:

    if (pszDomain)
    {
        LWNetFreeString(pszDomain);
    }

    return ntStatus;

error:

    *ppNewHostInfo = NULL;

    if (pHostInfo)
    {
        SrvFreeHostInfo(pHostInfo);
    }

    goto cleanup;
}

VOID
SrvReleaseHostInfo(
    PSRV_HOST_INFO pHostinfo
    )
{
    if (InterlockedDecrement(&pHostinfo->refcount) == 0)
    {
        SrvFreeHostInfo(pHostinfo);
    }
}

static
VOID
SrvFreeHostInfo(
    PSRV_HOST_INFO pHostinfo
    )
{
    if (pHostinfo->pMutex)
    {
        pthread_rwlock_destroy(&pHostinfo->mutex);
        pHostinfo->pMutex = NULL;
    }
    SMB_SAFE_FREE_STRING(pHostinfo->pszHostname);
    SMB_SAFE_FREE_STRING(pHostinfo->pszDomain);

    LwRtlMemoryFree(pHostinfo);
}
