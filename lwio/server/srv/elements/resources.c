/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

/*
 * Copyright (c) Likewise Software.  All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or (at
 * your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * for more details.  You should have received a copy of the GNU General
 * Public License along with this program.  If not, see
 * <http://www.gnu.org/licenses/>.
 *
 * LIKEWISE SOFTWARE MAKES THIS SOFTWARE AVAILABLE UNDER OTHER LICENSING
 * TERMS AS WELL.  IF YOU HAVE ENTERED INTO A SEPARATE LICENSE AGREEMENT
 * WITH LIKEWISE SOFTWARE, THEN YOU MAY ELECT TO USE THE SOFTWARE UNDER THE
 * TERMS OF THAT SOFTWARE LICENSE AGREEMENT INSTEAD OF THE TERMS OF THE GNU
 * GENERAL PUBLIC LICENSE, NOTWITHSTANDING THE ABOVE NOTICE.  IF YOU
 * HAVE QUESTIONS, OR WISH TO REQUEST A COPY OF THE ALTERNATE LICENSING
 * TERMS OFFERED BY LIKEWISE SOFTWARE, PLEASE CONTACT LIKEWISE SOFTWARE AT
 * license@likewise.com
 */

/*
 * Copyright (C) Likewise Software. All rights reserved.
 *
 * Module Name:
 *
 *        resources.c
 *
 * Abstract:
 *
 *        Likewise IO (LWIO) - SRV
 *
 *        Elements API
 *
 *        Resources
 *
 * Authors: Sriram Nambakam (snambakam@likewise.com)
 *
 */

#include "includes.h"

static
int
SrvElementsResourceCompare(
    PVOID pKey1,
    PVOID pKey2
    );

static
NTSTATUS
SrvElementsResourceAcquireId_inlock(
    PSRV_ELEMENTS_RESOURCES pResources,
    PULONG                  pulResourceId
    );

static
NTSTATUS
SrvElementsEnumResourcesInternal(
    PVOID    pKey,
    PVOID    pData,
    PVOID    pUserData,
    PBOOLEAN pbContinue
    );

NTSTATUS
SrvElementsResourcesInit(
    VOID
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;

    memset( &gSrvElements.resources,
            0,
            sizeof(gSrvElements.resources));

    pthread_rwlock_init(&gSrvElements.resources.mutex, NULL);
    gSrvElements.resources.pMutex = &gSrvElements.resources.mutex;

    ntStatus = LwRtlRBTreeCreate(
                    &SrvElementsResourceCompare,
                    NULL,
                    NULL,
                    &gSrvElements.resources.pResources);
    BAIL_ON_NT_STATUS(ntStatus);

error:

    return ntStatus;
}

static
int
SrvElementsResourceCompare(
    PVOID pKey1,
    PVOID pKey2
    )
{
    PULONG pResourceId1 = (PULONG)pKey1;
    PULONG pResourceId2 = (PULONG)pKey2;

    if (*pResourceId1 > *pResourceId2)
    {
        return 1;
    }
    else if (*pResourceId1 < *pResourceId2)
    {
        return -1;
    }
    else
    {
        return 0;
    }
}

NTSTATUS
SrvElementsRegisterResource(
    PSRV_RESOURCE pResource,    /* IN OUT          */
    PULONG        pulResourceId /* IN OUT OPTIONAL */
    )
{
    NTSTATUS ntStatus     = 0;
    BOOLEAN  bInLock      = FALSE;

    LWIO_LOCK_RWMUTEX_EXCLUSIVE(bInLock, &gSrvElements.resources.mutex);

    ntStatus = SrvElementsResourceAcquireId_inlock(
                    &gSrvElements.resources,
                    &pResource->ulResourceId);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = LwRtlRBTreeAdd(
                    gSrvElements.resources.pResources,
                    &pResource->ulResourceId,
                    pResource);
    BAIL_ON_NT_STATUS(ntStatus);

    if (pulResourceId)
    {
        *pulResourceId = pResource->ulResourceId;
    }

cleanup:

    LWIO_UNLOCK_RWMUTEX(bInLock, &gSrvElements.resources.mutex);

    return ntStatus;

error:

    if (pulResourceId)
    {
        *pulResourceId = 0;
    }

    pResource->ulResourceId = 0;

    goto cleanup;
}

static
NTSTATUS
SrvElementsResourceAcquireId_inlock(
    PSRV_ELEMENTS_RESOURCES pResources,
    PULONG                  pulResourceId
    )
{
    NTSTATUS ntStatus = 0;
    ULONG    ulResourceId = pResources->ulNextAvailableId;
    BOOLEAN  bFound = FALSE;

    do
    {
        PSRV_RESOURCE pResource = NULL;

        /* 0 is never a valid resource id */

        if ((ulResourceId == 0) || (ulResourceId == UINT32_MAX))
        {
            ulResourceId = 1;
        }

        ntStatus = LwRtlRBTreeFind(
                        pResources->pResources,
                        &ulResourceId,
                        (PVOID*)&pResource);
        if (ntStatus == STATUS_NOT_FOUND)
        {
            ntStatus = STATUS_SUCCESS;
            bFound = TRUE;
        }
        else
        {
            ulResourceId++;
        }
        BAIL_ON_NT_STATUS(ntStatus);

    } while ((ulResourceId != pResources->ulNextAvailableId)
                && !bFound);

    if (!bFound)
    {
        ntStatus = STATUS_TOO_MANY_OPENED_FILES;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    *pulResourceId = ulResourceId;

    /* Increment by 1 by make sure to deal with wrap-around */

    ulResourceId++;
    pResources->ulNextAvailableId = ulResourceId ? ulResourceId : 1;

cleanup:

    return ntStatus;

error:

    *pulResourceId = 0;

    goto cleanup;
}

NTSTATUS
SrvElementsFindResource(
    ULONG              ulResourceId,
    SRV_RESOURCE_TYPE  resourceType,
    PFN_ENUM_RESOURCES pfnEnumResourcesCB,
    PVOID              pUserData
    )
{
    NTSTATUS      ntStatus  = STATUS_SUCCESS;
    BOOLEAN       bInLock   = FALSE;
    PSRV_RESOURCE pResource = NULL;
    BOOLEAN       bContinue = FALSE;

    if ((resourceType == SRV_RESOURCE_TYPE_UNKNOWN) || !pfnEnumResourcesCB)
    {
        ntStatus = STATUS_INVALID_PARAMETER;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    LWIO_LOCK_RWMUTEX_SHARED(bInLock, &gSrvElements.resources.mutex);

    ntStatus = LwRtlRBTreeFind(
                    gSrvElements.resources.pResources,
                    &ulResourceId,
                    (PVOID*)&pResource);
    BAIL_ON_NT_STATUS(ntStatus);

    if (pResource->resourceType != resourceType)
    {
        ntStatus = STATUS_NOT_FOUND;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    ntStatus = pfnEnumResourcesCB(pResource, pUserData, &bContinue);

cleanup:

    LWIO_UNLOCK_RWMUTEX(bInLock, &gSrvElements.resources.mutex);

    return ntStatus;

error:

    goto cleanup;
}

NTSTATUS
SrvElementsEnumResources(
    SRV_RESOURCE_TYPE  resourceType,
    PFN_ENUM_RESOURCES pfnEnumResourcesCB,
    PVOID              pUserData
    )
{
    NTSTATUS ntStatus  = STATUS_SUCCESS;
    BOOLEAN  bInLock   = FALSE;
    SRV_ELEMENTS_ENUM_RESOURCES enumResources =
    {
        .resourceType       = resourceType,
        .pfnEnumResourcesCB = pfnEnumResourcesCB,
        .pUserData          = pUserData,
        .bContinue          = TRUE
    };

    if ((resourceType == SRV_RESOURCE_TYPE_UNKNOWN) || !pfnEnumResourcesCB)
    {
        ntStatus = STATUS_INVALID_PARAMETER;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    LWIO_LOCK_RWMUTEX_SHARED(bInLock, &gSrvElements.resources.mutex);

    ntStatus = LwRtlRBTreeTraverse(
                    gSrvElements.resources.pResources,
                    LWRTL_TREE_TRAVERSAL_TYPE_IN_ORDER,
                    &SrvElementsEnumResourcesInternal,
                    &enumResources);
    BAIL_ON_NT_STATUS(ntStatus);

cleanup:

    LWIO_UNLOCK_RWMUTEX(bInLock, &gSrvElements.resources.mutex);

    return ntStatus;

error:

    goto cleanup;
}

static
NTSTATUS
SrvElementsEnumResourcesInternal(
    PVOID    pKey,
    PVOID    pData,
    PVOID    pUserData,
    PBOOLEAN pbContinue
    )
{
    NTSTATUS      ntStatus  = STATUS_SUCCESS;
    PSRV_RESOURCE pResource = (PSRV_RESOURCE)pData;
    PSRV_ELEMENTS_ENUM_RESOURCES pEnumResources =
                                    (PSRV_ELEMENTS_ENUM_RESOURCES)pUserData;

    if (pResource->resourceType == pEnumResources->resourceType)
    {
        ntStatus = pEnumResources->pfnEnumResourcesCB(
                                        pResource,
                                        pEnumResources->pUserData,
                                        &pEnumResources->bContinue);
        BAIL_ON_NT_STATUS(ntStatus);
    }

    *pbContinue = pEnumResources->bContinue;

cleanup:

    return ntStatus;

error:

    *pbContinue = FALSE;

    goto cleanup;
}

NTSTATUS
SrvElementsUnregisterResource(
    ULONG          ulResourceId, /* IN             */
    PSRV_RESOURCE* ppResource    /*    OUT OPTIONAL*/
    )
{
    NTSTATUS      ntStatus  = STATUS_SUCCESS;
    BOOLEAN       bInLock   = FALSE;
    PSRV_RESOURCE pResource = NULL;

    LWIO_LOCK_RWMUTEX_EXCLUSIVE(bInLock, &gSrvElements.resources.mutex);

    if (ppResource)
    {
        ntStatus = LwRtlRBTreeFind(
                        gSrvElements.resources.pResources,
                        &ulResourceId,
                        (PVOID*)&pResource);
        BAIL_ON_NT_STATUS(ntStatus);
    }

    ntStatus = LwRtlRBTreeRemove(
                    gSrvElements.resources.pResources,
                    &ulResourceId);
    BAIL_ON_NT_STATUS(ntStatus);

    if (ppResource)
    {
        *ppResource = pResource;
    }

cleanup:

    LWIO_UNLOCK_RWMUTEX(bInLock, &gSrvElements.resources.mutex);

    return ntStatus;

error:

    if (ppResource)
    {
        *ppResource = NULL;
    }

    goto cleanup;
}

VOID
SrvElementsResourcesShutdown(
    VOID
    )
{
    if (gSrvElements.resources.pResources)
    {
        LwRtlRBTreeFree(gSrvElements.resources.pResources);
        gSrvElements.resources.pResources = NULL;
    }

    if (gSrvElements.resources.pMutex)
    {
        pthread_rwlock_destroy(&gSrvElements.resources.mutex);
        gSrvElements.resources.pMutex = NULL;
    }
}
