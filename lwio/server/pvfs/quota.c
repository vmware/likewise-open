/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

/*
 * Copyright Likewise Software
 * All rights reserved.
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
 * license@likewisesoftware.com
 */

/*
 * Copyright (C) Likewise Software. All rights reserved.
 *
 * Module Name:
 *
 *        quota.c
 *
 * Abstract:
 *
 *        Likewise Posix File System Driver (PVFS)
 *
 *        Query/Set Quota dispatch driver.  Implementation of a demo quota
 *        system.  Quota is not enforced.  All quota entries are held
 *        in memory.  All values are in host byte order.
 *
 *        Ideally, the quota enumeration must be taken from the underlying
 *        object store on which the file/directory indicated by pCcb resides.
 *        This demo implementation uses global in-memory quota information,
 *        which is not bound to the underlying object store.
 *
 *        For deletion of quota definitions, whose usage is greater than zero,
 *        from windows clients, the driver has to support
 *        FSCTL_FIND_FILES_BY_SID.
 *
 *        If the underlying FS supports quotas, the FileFsSizeInformation and
 *        FileFsFullSizeInformation handlers should take quota into account.
 *
 * Authors: Evgeny Popovich <epopovich@likewise.com>
 */

#include "pvfs.h"

/* Forward declarations */
static
NTSTATUS
PvfsQuotaInitQuotas();

static
NTSTATUS
PvfsQuotaAppendQuotaEntry(
    PPVFS_QUOTA_ENTRY        pQuotaEntry,
    PBYTE*                   ppBuffer,
    PULONG                   pulLength,
    PFILE_QUOTA_INFORMATION* ppAppendedEntry
    );

static
NTSTATUS
PvfsQuotaEnumerateQuotas(
    PPVFS_QUOTA_ENTRY pStartQuota,
    PBYTE*  ppBuffer,
    PULONG  pulLength,
    PIRP    pIrp,
    BOOLEAN bReturnSingleEntry
    );

static
NTSTATUS
PvfsQuotaEnumerateQuotasFromSidList(
    PBYTE  pSidList,
    PBYTE* ppBuffer,
    PULONG pulLength,
    PIRP   pIrp,
    BOOLEAN bReturnSingleEntry
    );

static
PPVFS_QUOTA_ENTRY
PvfsQuotaFindQuota(
    PBYTE   pSid,
    ULONG   ulSidLength
    );

static
VOID
PvfsQuotaFreeQuotaEntries(
    PPVFS_QUOTA_ENTRY pQuotaEntries
    );

static
NTSTATUS
PvfsQuotaUpdateQuotaEntry(
    PPVFS_QUOTA_ENTRY pQuotaEntry,
    PFILE_QUOTA_INFORMATION pQuotaInfo);

static
NTSTATUS
PvfsQuotaDeleteEntry(
    PPVFS_QUOTA_ENTRY* ppQuotaEntry
    );

static
NTSTATUS
PvfsQuotaSanityCheck(
    PIRP pIrp,
    ACCESS_MASK desiredAccessMask
    );

/* File Globals */
static PPVFS_QUOTA_ENTRY gpQuotas = NULL;
static PPVFS_QUOTA_ENTRY gpLastQuota = NULL;
static ULONG gulQuotasSize = 0;
static pthread_mutex_t gPvfsQuotaMutex;

#define PVFS_MAX_QUOTAS 100     // Limit memory consumption in demo code

/* Code */

NTSTATUS
PvfsQueryQuota(
    PPVFS_IRP_CONTEXT  pIrpContext
    )
{
    NTSTATUS ntError = STATUS_UNSUCCESSFUL;
    BOOLEAN bEnumerateAll = FALSE;
    PIRP pIrp = pIrpContext->pIrp;
    PPVFS_QUOTA_ENTRY pStartQuota = NULL;
    BOOLEAN bMutexLocked = FALSE;

    PBYTE pBuffer = (PBYTE)pIrp->Args.QueryQuota.Buffer;
    ULONG ulLength = pIrp->Args.QueryQuota.Length;
    BOOLEAN bReturnSingleEntry = pIrp->Args.QueryQuota.ReturnSingleEntry;
    PBYTE pSidList = (PBYTE)pIrp->Args.QueryQuota.SidList;
    ULONG ulSidListLength = pIrp->Args.QueryQuota.SidListLength;
    PFILE_GET_QUOTA_INFORMATION pStartSid = pIrp->Args.QueryQuota.StartSid;
    BOOLEAN bRestartScan = pIrp->Args.QueryQuota.RestartScan;

    ntError = PvfsQuotaSanityCheck(pIrp, FILE_GENERIC_READ);
    BAIL_ON_NT_STATUS(ntError);

    BAIL_ON_INVALID_PTR(pBuffer, ntError);

    if ((pSidList && ulSidListLength % 4) ||
        (ulSidListLength && pStartSid) ||
        (ulSidListLength && !pSidList) ||
        (ulSidListLength && ulSidListLength < 4))
    {
        ntError = STATUS_INVALID_PARAMETER;
        BAIL_ON_NT_STATUS(ntError);
    }

    /* Do some work */

    LWIO_LOCK_MUTEX(bMutexLocked, &gPvfsQuotaMutex);

    if (!ulSidListLength && !pStartSid)
    {
        bEnumerateAll = TRUE;
    }

    pIrp->IoStatusBlock.BytesTransferred = 0;
    if (bEnumerateAll)
    {
        pStartQuota = (bRestartScan) ? gpQuotas : gpLastQuota;

        ntError = PvfsQuotaEnumerateQuotas(
                        pStartQuota,
                        &pBuffer,
                        &ulLength,
                        pIrp,
                        bReturnSingleEntry);
        BAIL_ON_NT_STATUS(ntError);
    }
    else
    {
        if (ulSidListLength)
        {
            // We have a list of requested SIDs - return info only on them
            ntError = PvfsQuotaEnumerateQuotasFromSidList(
                            pSidList,
                            &pBuffer,
                            &ulLength,
                            pIrp,
                            bReturnSingleEntry);
            BAIL_ON_NT_STATUS(ntError);
        }
        else
        {
            // We have a start SID - enumerate starting from the start SID
            pStartQuota = PvfsQuotaFindQuota(pStartSid->Sid,
                                             pStartSid->SidLength);
            if (!pStartQuota)
            {
                ntError = STATUS_INVALID_SID;
                BAIL_ON_NT_STATUS(ntError);
            }

            ntError = PvfsQuotaEnumerateQuotas(
                        pStartQuota,
                        &pBuffer,
                        &ulLength,
                        pIrp,
                        bReturnSingleEntry);
            BAIL_ON_NT_STATUS(ntError);
        }
    }

    ntError = STATUS_SUCCESS;

cleanup:

    LWIO_UNLOCK_MUTEX(bMutexLocked, &gPvfsQuotaMutex);

    return ntError;

error:

    goto cleanup;
}

static
NTSTATUS
PvfsQuotaEnumerateQuotasFromSidList(
    PBYTE   pSidList,
    PBYTE*  ppBuffer,
    PULONG  pulLength,
    PIRP    pIrp,
    BOOLEAN bReturnSingleEntry
    )
{
    NTSTATUS ntError = STATUS_UNSUCCESSFUL;
    PFILE_GET_QUOTA_INFORMATION pGetQuotaInfo = NULL;
    PPVFS_QUOTA_ENTRY pQuotaEntry = NULL;
    PFILE_QUOTA_INFORMATION pLastEntry = NULL;

    static PVFS_QUOTA_ENTRY emptyQuota = { 0 };

    for (;;)
    {
        pGetQuotaInfo = (PFILE_GET_QUOTA_INFORMATION)pSidList;
        pQuotaEntry = PvfsQuotaFindQuota(pGetQuotaInfo->Sid,
                                         pGetQuotaInfo->SidLength);
        if (!pQuotaEntry)
        {
            emptyQuota.SidLength = pGetQuotaInfo->SidLength;
            RtlCopyMemory(emptyQuota.Sid,
                          pGetQuotaInfo->Sid,
                          pGetQuotaInfo->SidLength);
            pQuotaEntry = &emptyQuota;
        }

        ntError = PvfsQuotaAppendQuotaEntry(
                        pQuotaEntry,
                        ppBuffer,
                        pulLength,
                        &pLastEntry);
        BAIL_ON_NT_STATUS(ntError); // Also on STATUS_BUFFER_TOO_SMALL

        if (pGetQuotaInfo->NextEntryOffset == 0 || bReturnSingleEntry)
        {
            break;
        }
        else
        {
            pSidList += pGetQuotaInfo->NextEntryOffset;
        }
    }

    if (pLastEntry)
    {
        pLastEntry->NextEntryOffset = 0;
        pIrp->IoStatusBlock.BytesTransferred =
                            *ppBuffer - (PBYTE)pIrp->Args.QueryQuota.Buffer;
    }

    ntError = STATUS_SUCCESS;

error:

    return ntError;
}

NTSTATUS
PvfsSetQuota(
    PPVFS_IRP_CONTEXT  pIrpContext
    )
{
    NTSTATUS ntError = STATUS_UNSUCCESSFUL;
    PIRP pIrp = pIrpContext->pIrp;
    PBYTE pBuffer = (PBYTE)pIrp->Args.SetQuota.Buffer;
    ULONG ulLength = pIrp->Args.SetQuota.Length;
    PPVFS_QUOTA_ENTRY pNewEntries = NULL;
    PPVFS_QUOTA_ENTRY pNewEntriesTail = NULL;
    PPVFS_QUOTA_ENTRY pNewQuotaEntry = NULL;
    ULONG ulNewEntriesSize = 0;
    BOOLEAN bMutexLocked = FALSE;

    ntError = PvfsQuotaSanityCheck(pIrp, FILE_GENERIC_READ | FILE_GENERIC_WRITE);
    BAIL_ON_NT_STATUS(ntError);

    BAIL_ON_INVALID_PTR(pBuffer, ntError);

    /* Do some work */

    LWIO_LOCK_MUTEX(bMutexLocked, &gPvfsQuotaMutex);

    while (ulLength)
    {
        PFILE_QUOTA_INFORMATION pQuotaInfo = (PFILE_QUOTA_INFORMATION)pBuffer;
        ULONG ulMinQuotaInfoSize = (LW_FIELD_OFFSET(FILE_QUOTA_INFORMATION, Sid) +
                                    SID_MIN_SIZE);
        PPVFS_QUOTA_ENTRY pQuotaEntry = NULL;

        if (ulLength < ulMinQuotaInfoSize ||
            (pQuotaInfo->NextEntryOffset > 0 &&
             pQuotaInfo->NextEntryOffset < ulMinQuotaInfoSize))
        {
            ntError = STATUS_INVALID_PARAMETER;
            BAIL_ON_NT_STATUS(ntError);
        }

        if ((pQuotaEntry = PvfsQuotaFindQuota(pQuotaInfo->Sid, pQuotaInfo->SidLength)))
        {
            if (pQuotaInfo->QuotaLimit == -2)
            {
                ntError = PvfsQuotaDeleteEntry(&pQuotaEntry);
                BAIL_ON_NT_STATUS(ntError);
            }
            else
            {
                ntError = PvfsQuotaUpdateQuotaEntry(pQuotaEntry, pQuotaInfo);
                BAIL_ON_NT_STATUS(ntError);
            }
        }
        else
        {
            if (pQuotaInfo->QuotaLimit == -2)
            {
                // Request to delete and the entry is not found - nothing to do
                goto cleanup;
            }

            // Add new entry
            if (gulQuotasSize >= PVFS_MAX_QUOTAS)
            {
                ntError = STATUS_INSUFFICIENT_RESOURCES;
                BAIL_ON_NT_STATUS(ntError);
            }

            ntError = PvfsAllocateMemory((PVOID*)&pNewQuotaEntry,
                                         sizeof(*pNewQuotaEntry),
                                         TRUE);
            BAIL_ON_NT_STATUS(ntError);

            ntError = PvfsQuotaUpdateQuotaEntry(pNewQuotaEntry, pQuotaInfo);
            BAIL_ON_NT_STATUS(ntError);

            if (!pNewEntries)
            {
                pNewEntries = pNewQuotaEntry;
                pNewEntriesTail = pNewQuotaEntry;
            }
            else
            {
                pNewQuotaEntry->pNext = pNewEntries;
                pNewEntries = pNewQuotaEntry;
            }
            ++ulNewEntriesSize;
        }

        if (!pQuotaInfo->NextEntryOffset)
        {
            break;
        }

        pBuffer += pQuotaInfo->NextEntryOffset;
        ulLength -= pQuotaInfo->NextEntryOffset;
    }

    if (pNewEntries)
    {
        pNewEntriesTail->pNext = gpQuotas;
        gpQuotas = pNewEntries;
        gulQuotasSize += ulNewEntriesSize;
    }

    pIrp->IoStatusBlock.BytesTransferred = 0;

cleanup:

    LWIO_UNLOCK_MUTEX(bMutexLocked, &gPvfsQuotaMutex);

    return ntError;

error:

    PvfsFreeMemory((PVOID*)&pNewQuotaEntry);
    PvfsQuotaFreeQuotaEntries(pNewEntries);

    goto cleanup;
}

VOID
PvfsShutdownQuota()
{
    BOOLEAN bMutexLocked = FALSE;

    LWIO_LOCK_MUTEX(bMutexLocked, &gPvfsQuotaMutex);

    PvfsQuotaFreeQuotaEntries(gpQuotas);

    gpQuotas = NULL;
    gulQuotasSize = 0;
    gpLastQuota = NULL;

    LWIO_UNLOCK_MUTEX(bMutexLocked, &gPvfsQuotaMutex);
}

static
NTSTATUS
PvfsQuotaInitQuotas()
{
    NTSTATUS ntError = STATUS_UNSUCCESSFUL;
    BOOLEAN bMutexLocked = FALSE;

    LWIO_LOCK_MUTEX(bMutexLocked, &gPvfsQuotaMutex);

    if (gpQuotas)
    {
        ntError = STATUS_SUCCESS;
        goto cleanup;
    }

    ntError = PvfsAllocateMemory(
                    (PVOID*)&gpQuotas,
                    sizeof(*gpQuotas),
                    TRUE);
    BAIL_ON_NT_STATUS(ntError);

    ntError = PvfsAllocateMemory(
                    (PVOID*)&gpQuotas->pNext,
                    sizeof(*gpQuotas),
                    TRUE);
    BAIL_ON_NT_STATUS(ntError);

    // Just for fun, add two quota definitions: Administrators and
    // Users

    gpQuotas->QuotaLimit = -1;
    gpQuotas->QuotaThreshold = -1;
    gpQuotas->QuotaUsed = 1024 * 1024;
    gpQuotas->SidLength = SID_MAX_SIZE;
    ntError = RtlCreateWellKnownSid(
                   WinBuiltinAdministratorsSid,
                   NULL,
                   (PSID)gpQuotas->Sid,
                   &gpQuotas->SidLength);
    BAIL_ON_NT_STATUS(ntError);

    gpQuotas->pNext->QuotaLimit = gPvfsFileFsControlInformation.DefaultQuotaLimit;
    gpQuotas->pNext->QuotaThreshold = gPvfsFileFsControlInformation.DefaultQuotaThreshold;
    gpQuotas->pNext->QuotaUsed = 2 * 1024 * 1024;
    gpQuotas->pNext->SidLength = SID_MAX_SIZE;
    ntError = RtlCreateWellKnownSid(
                   WinBuiltinUsersSid,
                   NULL,
                   (PSID)gpQuotas->pNext->Sid,
                   &gpQuotas->pNext->SidLength);
    BAIL_ON_NT_STATUS(ntError);

    gulQuotasSize = 2;
    gpLastQuota = gpQuotas;

    ntError = STATUS_SUCCESS;

cleanup:

    LWIO_UNLOCK_MUTEX(bMutexLocked, &gPvfsQuotaMutex);

    return ntError;

error:

    if (gpQuotas)
    {
        PvfsFreeMemory((PVOID*)&gpQuotas->pNext);
        PvfsFreeMemory((PVOID*)&gpQuotas);
    }

    gulQuotasSize = 0;
    gpLastQuota = NULL;

    goto cleanup;
}

static
NTSTATUS
PvfsQuotaAppendQuotaEntry(
    PPVFS_QUOTA_ENTRY        pQuotaEntry,
    PBYTE*                   ppBuffer,
    PULONG                   pulLength,
    PFILE_QUOTA_INFORMATION* ppAppendedEntry
    )
{
    NTSTATUS ntError = STATUS_UNSUCCESSFUL;
    ULONG    ulRequiredLength = 0;
    PFILE_QUOTA_INFORMATION pFileQuotaInfo = (PFILE_QUOTA_INFORMATION)*ppBuffer;
    ULONG    ulPaddingBytes = 0;

    if (!pQuotaEntry)
    {
        ntError = STATUS_INTERNAL_ERROR;
        BAIL_ON_NT_STATUS(ntError);
    }

    // Each entry must be aligned on an 8-byte boundary

    ulRequiredLength = LW_FIELD_OFFSET(FILE_QUOTA_INFORMATION, Sid) +
                       pQuotaEntry->SidLength;
    if (ulRequiredLength % 8)
    {
        ulPaddingBytes = 8 - ulRequiredLength % 8;
    }
    if ((ulRequiredLength + ulPaddingBytes) > *pulLength)
    {
        ntError = STATUS_BUFFER_TOO_SMALL;
        BAIL_ON_NT_STATUS(ntError);
    }
    if (ulPaddingBytes)
    {
        RtlZeroMemory((*ppBuffer) + ulRequiredLength, ulPaddingBytes);
        ulRequiredLength += ulPaddingBytes;
    }

    pFileQuotaInfo->NextEntryOffset = ulRequiredLength;
    pFileQuotaInfo->SidLength = pQuotaEntry->SidLength;
    pFileQuotaInfo->ChangeTime = pQuotaEntry->ChangeTime;
    pFileQuotaInfo->QuotaUsed = pQuotaEntry->QuotaUsed;
    pFileQuotaInfo->QuotaThreshold = pQuotaEntry->QuotaThreshold;
    pFileQuotaInfo->QuotaLimit = pQuotaEntry->QuotaLimit;
    RtlCopyMemory(pFileQuotaInfo->Sid,
                  pQuotaEntry->Sid,
                  pQuotaEntry->SidLength);

    *ppAppendedEntry = (PFILE_QUOTA_INFORMATION)*ppBuffer;
    *ppBuffer = *ppBuffer + ulRequiredLength;
    *pulLength = *pulLength - ulRequiredLength;

    ntError = STATUS_SUCCESS;

cleanup:

    return ntError;

error:

    goto cleanup;
}

static
NTSTATUS
PvfsQuotaEnumerateQuotas(
    PPVFS_QUOTA_ENTRY pStartQuota,
    PBYTE*  ppBuffer,
    PULONG  pulLength,
    PIRP    pIrp,
    BOOLEAN bReturnSingleEntry
    )
{
    NTSTATUS ntError = STATUS_UNSUCCESSFUL;
    PFILE_QUOTA_INFORMATION pLastEntry = NULL;

    gpLastQuota = pStartQuota;
    for (; gpLastQuota; gpLastQuota = gpLastQuota->pNext)
    {
        ntError = PvfsQuotaAppendQuotaEntry(
                        gpLastQuota,
                        ppBuffer,
                        pulLength,
                        &pLastEntry);
        if (ntError == STATUS_BUFFER_TOO_SMALL)
        {
            if (!pLastEntry)    // Failed on the first entry
            {
                BAIL_ON_NT_STATUS(ntError);
            }
            ntError = STATUS_SUCCESS;   // Allow partial result
            break;
        }
        BAIL_ON_NT_STATUS(ntError);

        if (bReturnSingleEntry)
        {
            gpLastQuota = gpLastQuota->pNext;
            break;
        }
    }

    if (pLastEntry)
    {
        pLastEntry->NextEntryOffset = 0;
        pIrp->IoStatusBlock.BytesTransferred =
                            *ppBuffer - (PBYTE)pIrp->Args.QueryQuota.Buffer;
    }

    ntError = STATUS_SUCCESS;

error:

    return ntError;
}

static
PPVFS_QUOTA_ENTRY
PvfsQuotaFindQuota(
    PBYTE   pSid,
    ULONG   ulSidLength
    )
{
    PPVFS_QUOTA_ENTRY it = NULL;

    for (it = gpQuotas; it != NULL; it = it->pNext)
    {
        if (it->SidLength != ulSidLength)
        {
            continue;
        }
        if (RtlEqualMemory(it->Sid, pSid, it->SidLength))
        {
            break;
        }
    }

    return it;
}

static
VOID
PvfsQuotaFreeQuotaEntries(
    PPVFS_QUOTA_ENTRY pQuotaEntries
    )
{
    while (pQuotaEntries)
    {
        PPVFS_QUOTA_ENTRY pNext = pQuotaEntries->pNext;
        PvfsFreeMemory((PVOID*)&pQuotaEntries);
        pQuotaEntries = pNext;
    }
}

static
NTSTATUS
PvfsQuotaUpdateQuotaEntry(
    PPVFS_QUOTA_ENTRY pQuotaEntry,
    PFILE_QUOTA_INFORMATION pQuotaInfo)
{
    NTSTATUS ntError = STATUS_UNSUCCESSFUL;

    pQuotaEntry->SidLength      = pQuotaInfo->SidLength;
    pQuotaEntry->ChangeTime     = pQuotaInfo->ChangeTime;
    pQuotaEntry->QuotaUsed      = pQuotaInfo->QuotaUsed;
    pQuotaEntry->QuotaThreshold = pQuotaInfo->QuotaThreshold;
    pQuotaEntry->QuotaLimit     = pQuotaInfo->QuotaLimit;

    if (pQuotaInfo->SidLength > SID_MAX_SIZE)
    {
        ntError = STATUS_INVALID_PARAMETER;
        BAIL_ON_NT_STATUS(ntError);
    }

    RtlCopyMemory(pQuotaEntry->Sid, pQuotaInfo->Sid, pQuotaInfo->SidLength);

    ntError = STATUS_SUCCESS;

cleanup:

    return ntError;

error:

    goto cleanup;
}

static
NTSTATUS
PvfsQuotaDeleteEntry(
    PPVFS_QUOTA_ENTRY* ppQuotaEntry
    )
{
    NTSTATUS ntError = STATUS_UNSUCCESSFUL;

    if (!gpQuotas)
    {
        ntError = STATUS_INTERNAL_ERROR;
        BAIL_ON_NT_STATUS(ntError);
    }

    if (*ppQuotaEntry == gpQuotas)
    {
        gpQuotas = gpQuotas->pNext;
    }
    else
    {
        PPVFS_QUOTA_ENTRY pFirstEntry = gpQuotas;
        PPVFS_QUOTA_ENTRY pSecondEntry = gpQuotas->pNext;

        while (pSecondEntry && pSecondEntry != *ppQuotaEntry)
        {
            pFirstEntry = pSecondEntry;
            pSecondEntry = pSecondEntry->pNext;
        }

        if (!pSecondEntry)
        {
            ntError = STATUS_INTERNAL_ERROR;
            BAIL_ON_NT_STATUS(ntError);
        }

        pFirstEntry->pNext = pSecondEntry->pNext;
    }

    if (*ppQuotaEntry == gpLastQuota)
    {
        gpLastQuota = gpQuotas;
    }

    PvfsFreeMemory((PVOID*)ppQuotaEntry);

    --gulQuotasSize;

    ntError = STATUS_SUCCESS;

cleanup:

    return ntError;

error:

    goto cleanup;
}

static
NTSTATUS
PvfsQuotaSanityCheck(
    PIRP pIrp,
    ACCESS_MASK desiredAccessMask
    )
{
    NTSTATUS ntError = STATUS_UNSUCCESSFUL;
    PPVFS_CCB pCcb = NULL;

    if (gpQuotas == NULL)
    {
        ntError = PvfsQuotaInitQuotas();
        BAIL_ON_NT_STATUS(ntError);
    }

    ntError = PvfsAcquireCCB(pIrp->FileHandle, &pCcb);
    BAIL_ON_NT_STATUS(ntError);

    if (!PVFS_IS_DIR(pCcb) || !pCcb->bQuotaFile)
    {
        ntError = STATUS_INVALID_INFO_CLASS;
        BAIL_ON_NT_STATUS(ntError);
    }

    ntError = PvfsAccessCheckFileHandle(pCcb, desiredAccessMask);
    BAIL_ON_NT_STATUS(ntError);

    ntError = STATUS_SUCCESS;

cleanup:

    if (pCcb) {
        PvfsReleaseCCB(pCcb);
    }

    return ntError;

error:

    goto cleanup;
}

/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/

