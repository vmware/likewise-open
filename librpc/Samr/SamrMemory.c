/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 */

/*
 * Copyright Likewise Software    2004-2008
 * All rights reserved.
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the license, or (at
 * your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of 
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser
 * General Public License for more details.  You should have received a copy
 * of the GNU Lesser General Public License along with this program.  If
 * not, see <http://www.gnu.org/licenses/>.
 *
 * LIKEWISE SOFTWARE MAKES THIS SOFTWARE AVAILABLE UNDER OTHER LICENSING
 * TERMS AS WELL.  IF YOU HAVE ENTERED INTO A SEPARATE LICENSE AGREEMENT
 * WITH LIKEWISE SOFTWARE, THEN YOU MAY ELECT TO USE THE SOFTWARE UNDER THE
 * TERMS OF THAT SOFTWARE LICENSE AGREEMENT INSTEAD OF THE TERMS OF THE GNU
 * LESSER GENERAL PUBLIC LICENSE, NOTWITHSTANDING THE ABOVE NOTICE.  IF YOU
 * HAVE QUESTIONS, OR WISH TO REQUEST A COPY OF THE ALTERNATE LICENSING
 * TERMS OFFERED BY LIKEWISE SOFTWARE, PLEASE CONTACT LIKEWISE SOFTWARE AT
 * license@likewisesoftware.com
 */

/*
 * Abstract: Samr memory (de)allocation routines (rpc client library)
 *
 * Authors: Rafal Szczesniak (rafal@likewisesoftware.com)
 */

#include "includes.h"


NTSTATUS
SamrInitMemory(
    void
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    BOOLEAN bLocked = FALSE;

    LIBRPC_LOCK_MUTEX(bLocked, &gSamrDataMutex);

    if (!bSamrInitialised)
    {
        ntStatus = MemPtrListInit((PtrList**)&gSamrMemoryList);
        BAIL_ON_NT_STATUS(ntStatus);

        bSamrInitialised = 1;
    }

cleanup:
    LIBRPC_UNLOCK_MUTEX(bLocked, &gSamrDataMutex);

    return ntStatus;

error:
    goto cleanup;
}


NTSTATUS
SamrDestroyMemory(
    void
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    int bLocked = 0;

    LIBRPC_LOCK_MUTEX(bLocked, &gSamrDataMutex);

    if (bSamrInitialised && gSamrMemoryList) {
        ntStatus = MemPtrListDestroy((PtrList**)&gSamrMemoryList);
        BAIL_ON_NT_STATUS(ntStatus);

        bSamrInitialised = 0;
    }

cleanup:
    LIBRPC_UNLOCK_MUTEX(bLocked, &gSamrDataMutex);

    return ntStatus;

error:
    goto cleanup;
}


NTSTATUS
SamrAllocateMemory(
    OUT PVOID  *ppOut,
    IN  size_t  Size,
    IN  PVOID   pDep
    )
{
    return MemPtrAllocate((PtrList*)gSamrMemoryList,
                          ppOut,
                          Size,
                          pDep);
}


NTSTATUS
SamrFreeMemory(
    IN  PVOID pPtr
    )
{
    return MemPtrFree((PtrList*)gSamrMemoryList,
                      pPtr);
}


NTSTATUS
SamrAddDepMemory(
    IN  PVOID pPtr,
    IN  PVOID pDep
    )
{
    return MemPtrAddDependant((PtrList*)gSamrMemoryList,
                              pPtr,
                              pDep);
}


NTSTATUS
SamrAllocateNamesAndRids(
    OUT PWSTR        **pppNames,
    OUT UINT32       **ppRids,
    IN  RidNameArray  *pIn
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PWSTR *ppNames = NULL;
    UINT32 *pRids = NULL;
    UINT32 iName = 0;

    BAIL_ON_INVALID_PTR(pppNames, ntStatus);
    BAIL_ON_INVALID_PTR(ppRids, ntStatus);
    BAIL_ON_INVALID_PTR(pIn, ntStatus);

    ntStatus = SamrAllocateMemory((void**)&ppNames,
                                  sizeof(PWSTR) * pIn->count,
                                  NULL);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SamrAllocateMemory((void**)&pRids,
                                  sizeof(UINT32) * pIn->count,
                                  NULL);
    BAIL_ON_NT_STATUS(ntStatus);

    for (iName = 0; iName < pIn->count; iName++) {
        RidName *pRidName = &(pIn->entries[iName]);
        
        pRids[iName]   = pRidName->rid;
        ppNames[iName] = GetFromUnicodeString(&pRidName->name);
        BAIL_ON_NULL_PTR(ppNames[iName], ntStatus);
        
        ntStatus = SamrAddDepMemory((void*)ppNames[iName],
                                    (void*)ppNames);
        BAIL_ON_NT_STATUS(ntStatus);
    }

    *pppNames = ppNames;
    *ppRids   = pRids;

cleanup:
    return ntStatus;

error:
    if (ppNames) {
        SamrFreeMemory((void*)ppNames);
    }

    if (pRids) {
        SamrFreeMemory((void*)pRids);
    }

    *pppNames = NULL;
    *ppRids = NULL;

    goto cleanup;
}


NTSTATUS
SamrAllocateNames(
    OUT PWSTR **pppwszNames,
    IN  EntryArray *pNamesArray
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PWSTR *ppwszNames = NULL;
    UINT32 iName = 0;

    BAIL_ON_INVALID_PTR(pppwszNames, ntStatus);
    BAIL_ON_INVALID_PTR(pNamesArray, ntStatus);

    ntStatus = SamrAllocateMemory((void**)&ppwszNames,
                                  sizeof(PWSTR) * pNamesArray->count,
                                  NULL);
    BAIL_ON_NT_STATUS(ntStatus);

    for (iName = 0; iName < pNamesArray->count; iName++) {
        Entry *pName = &(pNamesArray->entries[iName]);

        ppwszNames[iName] = GetFromUnicodeString(&pName->name);
        BAIL_ON_NULL_PTR(ppwszNames[iName], ntStatus);

        ntStatus = SamrAddDepMemory((void*)ppwszNames[iName],
                                    (void*)ppwszNames);
        BAIL_ON_NT_STATUS(ntStatus);
    }

    *pppwszNames = ppwszNames;

cleanup:
    return ntStatus;

error:
    if (ppwszNames) {
        SamrFreeMemory((void*)ppwszNames);
    }

    *pppwszNames = NULL;

    goto cleanup;
}


NTSTATUS
SamrAllocateIds(
    OUT UINT32 **ppOutIds,
    IN  Ids *pInIds
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    UINT32 *pIds = NULL;
    UINT32 i = 0;

    BAIL_ON_INVALID_PTR(ppOutIds, ntStatus);
    BAIL_ON_INVALID_PTR(pInIds, ntStatus);

    ntStatus = SamrAllocateMemory((void**)&pIds,
                                  sizeof(UINT32) * pInIds->count,
                                  NULL);
    BAIL_ON_NT_STATUS(ntStatus);

    for (i = 0; i < pInIds->count; i++) {
        pIds[i] = pInIds->ids[i];
    }

    *ppOutIds = pIds;

cleanup:
    return ntStatus;

error:
    if (pIds) {
        SamrFreeMemory((void*)pIds);
    }

    *ppOutIds = NULL;

    goto cleanup;
}


NTSTATUS
SamrAllocateDomSid(
    OUT PSID *ppOut,
    IN  PSID  pIn,
    IN  PVOID pDep
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PSID pSid = NULL;

    BAIL_ON_INVALID_PTR(ppOut, ntStatus);

    MsRpcDuplicateSid(&pSid, pIn);
    BAIL_ON_NULL_PTR(pSid, ntStatus);

    ntStatus = SamrAddDepMemory((void*)pSid, (void*)pDep);
    BAIL_ON_NT_STATUS(ntStatus);

    *ppOut = pSid;

cleanup:
    return ntStatus;

error:
    if (pSid) {
        SamrFreeMemory((void*)pSid);
    }

    *ppOut = NULL;

    goto cleanup;
}


NTSTATUS
SamrAllocateSids(
    OUT PSID     **pppSids,
    IN  SidArray  *pSidArray
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PSID *ppSids = NULL;
    int i = 0;

    BAIL_ON_INVALID_PTR(pppSids, ntStatus);
    BAIL_ON_INVALID_PTR(pSidArray, ntStatus);

    ntStatus = SamrAllocateMemory((void**)&ppSids,
                                  sizeof(PSID) * pSidArray->num_sids,
                                  NULL);
    BAIL_ON_NT_STATUS(ntStatus);

    for (i = 0; i < pSidArray->num_sids; i++) {
        PSID pSid = pSidArray->sids[i].sid;

        ntStatus = SamrAllocateDomSid(&(ppSids[i]),
                                      pSid,
                                      (void*)ppSids);
        BAIL_ON_NT_STATUS(ntStatus);
    }

    *pppSids = ppSids;

cleanup:
    return ntStatus;

error:
    if (ppSids) {
        SamrFreeMemory((void*)ppSids);
    }

    *pppSids = NULL;

    goto cleanup;
}


NTSTATUS
SamrAllocateRidsAndAttributes(
    OUT UINT32                **ppRids,
    OUT UINT32                **ppAttributes,
    IN  RidWithAttributeArray  *pIn
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    UINT32 *pRids = NULL;
    UINT32 *pAttributes = NULL;
    int i = 0;

    BAIL_ON_INVALID_PTR(ppRids, ntStatus);
    BAIL_ON_INVALID_PTR(ppAttributes, ntStatus);
    BAIL_ON_INVALID_PTR(pIn, ntStatus);

    ntStatus = SamrAllocateMemory((void**)&pRids,
                                  sizeof(UINT32) * pIn->count,
                                  NULL);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SamrAllocateMemory((void**)&pAttributes,
                                  sizeof(UINT32) * pIn->count,
                                  NULL);
    BAIL_ON_NT_STATUS(ntStatus);

    for (i = 0; i < pIn->count; i++) {
        RidWithAttribute *pRidAttr = &(pIn->rids[i]);

        pRids[i]       = pRidAttr->rid;
        pAttributes[i] = pRidAttr->attributes;
    }

    *ppRids       = pRids;
    *ppAttributes = pAttributes;

cleanup:
    return ntStatus;

error:
    if (pRids) {
        SamrFreeMemory((void*)pRids);
    }

    if (pAttributes) {
        SamrFreeMemory((void*)pAttributes);
    }

    *ppRids       = NULL;
    *ppAttributes = NULL;

    goto cleanup;
}


static
NTSTATUS
SamrCopyUnicodeString(
    OUT UnicodeString *pOut,
    IN  UnicodeString *pIn,
    IN  PVOID pDep
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;

    BAIL_ON_INVALID_PTR(pOut, ntStatus);
    BAIL_ON_INVALID_PTR(pIn, ntStatus);

    ntStatus = CopyUnicodeString(pOut, pIn);
    BAIL_ON_NT_STATUS(ntStatus);

    if (pOut->string) {
        ntStatus = SamrAddDepMemory((void*)pOut->string,
                                    pDep);
        BAIL_ON_NT_STATUS(ntStatus);
    }

cleanup:
    return ntStatus;

error:
    goto cleanup;
}


NTSTATUS
SamrAllocateAliasInfo(
    OUT AliasInfo **ppOut,
    IN  AliasInfo  *pIn,
    IN  UINT16      Level
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    AliasInfo *pInfo = NULL;

    BAIL_ON_INVALID_PTR(ppOut, ntStatus);
    BAIL_ON_INVALID_PTR(pIn, ntStatus);

    ntStatus = SamrAllocateMemory((void*)&pInfo,
                                  sizeof(AliasInfo),
                                  NULL);
    BAIL_ON_NT_STATUS(ntStatus);

    switch (Level) {
    case ALIAS_INFO_ALL:
        ntStatus = SamrCopyUnicodeString(&pInfo->all.name,
                                         &pIn->all.name,
                                         (void*)pInfo);
        BAIL_ON_NT_STATUS(ntStatus);

        ntStatus = SamrCopyUnicodeString(&pInfo->all.description,
                                         &pIn->all.description,
                                         (void*)pInfo);
        BAIL_ON_NT_STATUS(ntStatus);

        pInfo->all.num_members = pIn->all.num_members;
        break;

    case ALIAS_INFO_NAME:
        ntStatus = SamrCopyUnicodeString(&pInfo->name,
                                         &pIn->name,
                                         (void*)pInfo);
        BAIL_ON_NT_STATUS(ntStatus);
        break;

    case ALIAS_INFO_DESCRIPTION:
        ntStatus = SamrCopyUnicodeString(&pInfo->description,
                                         &pIn->description,
                                         (void*)pInfo);
        BAIL_ON_NT_STATUS(ntStatus);
        break;

    default:
        ntStatus = STATUS_INVALID_LEVEL;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    *ppOut = pInfo;

cleanup:
    return ntStatus;

error:
    if (pInfo) {
        SamrFreeMemory((void*)pInfo);
    }

    *ppOut = NULL;

    goto cleanup;
}


static
NTSTATUS
SamrCopyDomainInfo1(
    OUT DomainInfo1 *pOut,
    IN  DomainInfo1 *pIn,
    IN  PVOID        pDep
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;

    BAIL_ON_INVALID_PTR(pOut, ntStatus);
    BAIL_ON_INVALID_PTR(pIn, ntStatus);

    memcpy((void*)pOut, (void*)pIn, sizeof(DomainInfo1));

cleanup:
    return ntStatus;

error:
    goto cleanup;
}


static
NTSTATUS
SamrCopyDomainInfo2(
    OUT DomainInfo2 *pOut,
    IN  DomainInfo2 *pIn,
    IN  PVOID        pDep
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;

    BAIL_ON_INVALID_PTR(pOut, ntStatus);
    BAIL_ON_INVALID_PTR(pIn, ntStatus);

    ntStatus = CopyUnicodeString(&pOut->comment,
                                 &pIn->comment);
    BAIL_ON_NT_STATUS(ntStatus);

    if (pOut->comment.string) {
        ntStatus = SamrAddDepMemory((void*)pOut->comment.string,
                                    pDep);
        BAIL_ON_NT_STATUS(ntStatus);
    }

    ntStatus = CopyUnicodeString(&pOut->domain_name, &pIn->domain_name);
    BAIL_ON_NT_STATUS(ntStatus);

    if (pOut->domain_name.string) {
        ntStatus = SamrAddDepMemory((void*)pOut->domain_name.string,
                                    pDep);
        BAIL_ON_NT_STATUS(ntStatus);
    }

    ntStatus = CopyUnicodeString(&pOut->primary,
                                 &pIn->primary);
    BAIL_ON_NT_STATUS(ntStatus);

    if (pOut->primary.string) {
        ntStatus = SamrAddDepMemory((void*)pOut->primary.string,
                                    pDep);
        BAIL_ON_NT_STATUS(ntStatus);
    }

    pOut->force_logoff_time = pIn->force_logoff_time;
    pOut->sequence_num      = pIn->sequence_num;
    pOut->unknown1          = pIn->unknown1;
    pOut->role              = pIn->role;
    pOut->unknown2          = pIn->unknown2;
    pOut->num_users         = pIn->num_users;
    pOut->num_groups        = pIn->num_groups;
    pOut->num_aliases       = pIn->num_aliases;

cleanup:
    return ntStatus;

error:
    goto cleanup;
}


static
NTSTATUS
SamrCopyDomainInfo3(
    OUT DomainInfo3 *pOut,
    IN  DomainInfo3 *pIn,
    IN  PVOID        pDep
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;

    BAIL_ON_INVALID_PTR(pOut, ntStatus);
    BAIL_ON_INVALID_PTR(pIn, ntStatus);

    pOut->force_logoff_time = pIn->force_logoff_time;

cleanup:
    return ntStatus;

error:
    goto cleanup;
}


static
NTSTATUS
SamrCopyDomainInfo4(
    OUT DomainInfo4 *pOut,
    IN  DomainInfo4 *pIn,
    IN  PVOID pDep
    )
{
    return SamrCopyUnicodeString(&pOut->comment,
                                 &pIn->comment,
                                 pDep);
}


static
NTSTATUS
SamrCopyDomainInfo5(
    OUT DomainInfo5 *pOut,
    IN  DomainInfo5 *pIn,
    IN  PVOID        pDep
    )
{
    return SamrCopyUnicodeString(&pOut->domain_name,
                                 &pIn->domain_name,
                                 pDep);
}


static
NTSTATUS
SamrCopyDomainInfo6(
    OUT DomainInfo6 *pOut,
    IN  DomainInfo6 *pIn,
    IN  PVOID        pDep
    )
{
    return SamrCopyUnicodeString(&pOut->primary,
                                 &pIn->primary,
                                 pDep);
}


static
NTSTATUS
SamrCopyDomainInfo7(
    OUT DomainInfo7 *pOut,
    IN  DomainInfo7 *pIn,
    IN  PVOID        pDep
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;

    BAIL_ON_INVALID_PTR(pOut, ntStatus);
    BAIL_ON_INVALID_PTR(pIn, ntStatus);

    pOut->role = pIn->role;

cleanup:
    return ntStatus;

error:
    goto cleanup;
}


static
NTSTATUS
SamrCopyDomainInfo8(
    OUT DomainInfo8 *pOut,
    IN  DomainInfo8 *pIn,
    IN  PVOID        pDep
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;

    BAIL_ON_INVALID_PTR(pOut, ntStatus);
    BAIL_ON_INVALID_PTR(pIn, ntStatus);

    pOut->sequence_number    = pIn->sequence_number;
    pOut->domain_create_time = pIn->domain_create_time;

cleanup:
    return ntStatus;

error:
    goto cleanup;
}


static
NTSTATUS
SamrCopyDomainInfo9(
    OUT DomainInfo9 *pOut,
    IN  DomainInfo9 *pIn,
    IN  PVOID        pDep
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;

    BAIL_ON_INVALID_PTR(pOut, ntStatus);
    BAIL_ON_INVALID_PTR(pIn, ntStatus);

    pOut->unknown = pIn->unknown;

cleanup:
    return ntStatus;

error:
    goto cleanup;
}


static
NTSTATUS
SamrCopyDomainInfo11(
    OUT DomainInfo11 *pOut,
    IN  DomainInfo11 *pIn,
    IN  PVOID         pDep
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;

    BAIL_ON_INVALID_PTR(pOut, ntStatus);
    BAIL_ON_INVALID_PTR(pIn, ntStatus);

    ntStatus = SamrCopyDomainInfo2(&pOut->info2,
                                   &pIn->info2,
                                   pDep);
    BAIL_ON_NT_STATUS(ntStatus);

    pOut->lockout_duration  = pIn->lockout_duration;
    pOut->lockout_window    = pIn->lockout_window;
    pOut->lockout_threshold = pIn->lockout_threshold;

cleanup:
    return ntStatus;

error:
    goto cleanup;
}


static
NTSTATUS
SamrCopyDomainInfo12(
    OUT DomainInfo12 *pOut,
    IN  DomainInfo12 *pIn,
    IN  PVOID         pDep
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;

    BAIL_ON_INVALID_PTR(pOut, ntStatus);
    BAIL_ON_INVALID_PTR(pIn, ntStatus);

    pOut->lockout_duration  = pIn->lockout_duration;
    pOut->lockout_window    = pIn->lockout_window;
    pOut->lockout_threshold = pIn->lockout_threshold;

cleanup:
    return ntStatus;

error:
    goto cleanup;
}


static
NTSTATUS
SamrCopyDomainInfo13(
    OUT DomainInfo13 *pOut,
    IN  DomainInfo13 *pIn,
    IN  PVOID         pDep
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;

    BAIL_ON_INVALID_PTR(pOut, ntStatus);
    BAIL_ON_INVALID_PTR(pIn, ntStatus);

    pOut->sequence_number    = pIn->sequence_number;
    pOut->domain_create_time = pIn->domain_create_time;
    pOut->unknown1           = pIn->unknown1;
    pOut->unknown2           = pIn->unknown2;

cleanup:
    return ntStatus;

error:
    goto cleanup;
}


NTSTATUS
SamrAllocateDomainInfo(
    OUT DomainInfo **ppOut,
    IN  DomainInfo *pIn,
    IN  UINT16 Level
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    DomainInfo *pInfo = NULL;

    BAIL_ON_INVALID_PTR(ppOut, ntStatus);
    BAIL_ON_INVALID_PTR(pIn, ntStatus);

    ntStatus = SamrAllocateMemory((void*)&pInfo,
                                  sizeof(DomainInfo),
                                  NULL);
    BAIL_ON_NT_STATUS(ntStatus);

    if (pIn != NULL) {
        switch (Level) {
        case 1:
            ntStatus = SamrCopyDomainInfo1(&pInfo->info1,
                                           &pIn->info1,
                                           (void*)pInfo);
            break;

        case 2:
            ntStatus = SamrCopyDomainInfo2(&pInfo->info2,
                                           &pIn->info2,
                                           (void*)pInfo);
            break;

        case 3:
            ntStatus = SamrCopyDomainInfo3(&pInfo->info3,
                                           &pIn->info3,
                                           (void*)pInfo);
            break;

        case 4:
            ntStatus = SamrCopyDomainInfo4(&pInfo->info4,
                                           &pIn->info4,
                                           (void*)pInfo);
            break;

        case 5:
            ntStatus = SamrCopyDomainInfo5(&pInfo->info5,
                                           &pIn->info5,
                                           (void*)pInfo);
            break;

        case 6:
            ntStatus = SamrCopyDomainInfo6(&pInfo->info6,
                                           &pIn->info6,
                                           (void*)pInfo);
            break;

        case 7:
            ntStatus = SamrCopyDomainInfo7(&pInfo->info7,
                                           &pIn->info7,
                                           (void*)pInfo);
            break;

        case 8:
            ntStatus = SamrCopyDomainInfo8(&pInfo->info8,
                                           &pIn->info8,
                                           (void*)pInfo);
            break;

        case 9:
            ntStatus = SamrCopyDomainInfo9(&pInfo->info9,
                                           &pIn->info9,
                                           (void*)pInfo);
            break;

        case 11:
            ntStatus = SamrCopyDomainInfo11(&pInfo->info11,
                                            &pIn->info11,
                                            (void*)pInfo);
            break;

        case 12:
            ntStatus = SamrCopyDomainInfo12(&pInfo->info12,
                                            &pIn->info12,
                                            (void*)pInfo);
            break;

        case 13:
            ntStatus = SamrCopyDomainInfo13(&pInfo->info13,
                                            &pIn->info13,
                                            (void*)pInfo);
            break;

        default:
            ntStatus = STATUS_INVALID_LEVEL;
            goto error;
        }

        BAIL_ON_NT_STATUS(ntStatus);
    }

    *ppOut = pInfo;

cleanup:
    return ntStatus;

error:
    if (pInfo) {
        SamrFreeMemory((void*)pInfo);
    }

    *ppOut = NULL;

    goto cleanup;
}


static
NTSTATUS
SamrCopyUserInfo1(
    OUT UserInfo1 *pOut,
    IN  UserInfo1 *pIn,
    IN  PVOID      pDep
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;

    BAIL_ON_INVALID_PTR(pOut, ntStatus);
    BAIL_ON_INVALID_PTR(pIn, ntStatus);

    ntStatus = SamrCopyUnicodeString(&pOut->account_name,
                                     &pIn->account_name,
                                     pDep);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SamrCopyUnicodeString(&pOut->full_name,
                                     &pIn->full_name,
                                     pDep);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SamrCopyUnicodeString(&pOut->description,
                                     &pIn->description,
                                     pDep);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SamrCopyUnicodeString(&pOut->comment,
                                     &pIn->comment,
                                     pDep);
    BAIL_ON_NT_STATUS(ntStatus);

    pOut->primary_gid = pIn->primary_gid;

cleanup:
    return ntStatus;

error:
    goto cleanup;
}


static
NTSTATUS
SamrCopyUserInfo2(
    OUT UserInfo2 *pOut,
    IN  UserInfo2 *pIn,
    IN  PVOID      pDep
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;

    BAIL_ON_INVALID_PTR(pOut, ntStatus);
    BAIL_ON_INVALID_PTR(pIn, ntStatus);

    ntStatus = SamrCopyUnicodeString(&pOut->comment,
                                     &pIn->comment,
                                     pDep);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SamrCopyUnicodeString(&pOut->unknown1,
                                     &pIn->unknown1,
                                     pDep);
    BAIL_ON_NT_STATUS(ntStatus);

    pOut->country_code = pIn->country_code;
    pOut->code_page    = pIn->code_page;

cleanup:
    return ntStatus;

error:
    goto cleanup;
}


static
NTSTATUS
SamrCopyLogonHours(
    OUT LogonHours *pOut,
    IN  LogonHours *pIn,
    IN  PVOID       pDep
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;

    BAIL_ON_INVALID_PTR(pOut, ntStatus);
    BAIL_ON_INVALID_PTR(pIn, ntStatus);

    /* Allocate 1260 bytes and copy units_per_week/8 bytes
       according to samr.idl */

    ntStatus = SamrAllocateMemory((void**)&pOut->units, sizeof(UINT8) * 1260, NULL);
    BAIL_ON_NT_STATUS(ntStatus);

    memcpy((void*)pOut->units, (void*)pIn->units, pIn->units_per_week/8);

cleanup:
    return ntStatus;

error:
    goto cleanup;
}


static
NTSTATUS
SamrCopyUserInfo3(
    OUT UserInfo3 *pOut,
    IN  UserInfo3 *pIn,
    IN  PVOID       pDep
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;

    BAIL_ON_INVALID_PTR(pOut, ntStatus);
    BAIL_ON_INVALID_PTR(pIn, ntStatus);

    ntStatus = SamrCopyUnicodeString(&pOut->account_name,
                                     &pIn->account_name,
                                     pDep);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SamrCopyUnicodeString(&pOut->full_name,
                                     &pIn->full_name,
                                     pDep);
    BAIL_ON_NT_STATUS(ntStatus);

    pOut->rid         = pIn->rid;
    pOut->primary_gid = pIn->primary_gid;

    ntStatus = SamrCopyUnicodeString(&pOut->home_directory,
                                     &pIn->home_directory,
                                     pDep);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SamrCopyUnicodeString(&pOut->home_drive,
                                     &pIn->home_drive,
                                     pDep);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SamrCopyUnicodeString(&pOut->logon_script,
                                     &pIn->logon_script,
                                     pDep);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SamrCopyUnicodeString(&pOut->profile_path,
                                     &pIn->profile_path,
                                     pDep);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SamrCopyUnicodeString(&pOut->workstations,
                                     &pIn->workstations,
                                     pDep);
    BAIL_ON_NT_STATUS(ntStatus);

    pOut->last_logon             = pIn->last_logon;
    pOut->last_logoff            = pIn->last_logoff;
    pOut->last_password_change   = pIn->last_password_change;
    pOut->allow_password_change  = pIn->allow_password_change;
    pOut->force_password_change  = pIn->force_password_change;

    ntStatus = SamrCopyLogonHours(&pOut->logon_hours,
                                  &pIn->logon_hours,
                                  pDep);
    BAIL_ON_NT_STATUS(ntStatus);

    pOut->bad_password_count = pIn->bad_password_count;
    pOut->logon_count        = pIn->logon_count;
    pOut->account_flags      = pIn->account_flags;

cleanup:
    return ntStatus;

error:
    goto cleanup;
}

static
NTSTATUS
SamrCopyUserInfo4(
    OUT UserInfo4 *pOut,
    IN  UserInfo4 *pIn,
    IN  PVOID      pDep
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;

    BAIL_ON_INVALID_PTR(pOut, ntStatus);
    BAIL_ON_INVALID_PTR(pIn, ntStatus);

    ntStatus = SamrCopyLogonHours(&pOut->logon_hours,
                                  &pIn->logon_hours,
                                  pDep);
    BAIL_ON_NT_STATUS(ntStatus);

cleanup:
    return ntStatus;

error:
    goto cleanup;
}


static
NTSTATUS
SamrCopyUserInfo5(
    OUT UserInfo5 *pOut,
    IN  UserInfo5 *pIn,
    IN  PVOID      pDep
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;

    BAIL_ON_INVALID_PTR(pOut, ntStatus);
    BAIL_ON_INVALID_PTR(pIn, ntStatus);

    ntStatus = SamrCopyUnicodeString(&pOut->account_name,
                                     &pIn->account_name,
                                     pDep);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SamrCopyUnicodeString(&pOut->full_name,
                                     &pIn->full_name,
                                     pDep);
    BAIL_ON_NT_STATUS(ntStatus);

    pOut->rid         = pIn->rid;
    pOut->primary_gid = pIn->primary_gid;

    ntStatus = SamrCopyUnicodeString(&pOut->home_directory,
                                     &pIn->home_directory,
                                     pDep);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SamrCopyUnicodeString(&pOut->home_drive,
                                     &pIn->home_drive,
                                     pDep);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SamrCopyUnicodeString(&pOut->logon_script,
                                     &pIn->logon_script,
                                     pDep);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SamrCopyUnicodeString(&pOut->profile_path,
                                     &pIn->profile_path,
                                     pDep);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SamrCopyUnicodeString(&pOut->description,
                                     &pIn->description,
                                     pDep);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SamrCopyUnicodeString(&pOut->workstations,
                                     &pIn->workstations,
                                     pDep);
    BAIL_ON_NT_STATUS(ntStatus);

    pOut->last_logon   = pIn->last_logon;
    pOut->last_logoff  = pIn->last_logoff;

    ntStatus = SamrCopyLogonHours(&pOut->logon_hours,
                                  &pIn->logon_hours,
                                  pDep);
    BAIL_ON_NT_STATUS(ntStatus);

    pOut->bad_password_count   = pIn->bad_password_count;
    pOut->logon_count          = pIn->logon_count;
    pOut->last_password_change = pIn->last_password_change;
    pOut->account_expiry       = pIn->account_expiry;
    pOut->account_flags        = pIn->account_flags;

cleanup:
    return ntStatus;

error:
    goto cleanup;
}


static
NTSTATUS
SamrCopyUserInfo6(
    OUT UserInfo6 *pOut,
    IN  UserInfo6 *pIn,
    IN  PVOID      pDep
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;

    BAIL_ON_INVALID_PTR(pOut, ntStatus);
    BAIL_ON_INVALID_PTR(pIn, ntStatus);

    ntStatus = SamrCopyUnicodeString(&pOut->account_name,
                                     &pIn->account_name,
                                     pDep);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SamrCopyUnicodeString(&pOut->full_name,
                                     &pIn->full_name,
                                     pDep);
    BAIL_ON_NT_STATUS(ntStatus);

cleanup:
    return ntStatus;

error:
    goto cleanup;
}


static
NTSTATUS
SamrCopyUserInfo7(
    OUT UserInfo7 *pOut,
    IN  UserInfo7 *pIn,
    IN  PVOID      pDep
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;

    BAIL_ON_INVALID_PTR(pOut, ntStatus);
    BAIL_ON_INVALID_PTR(pIn, ntStatus);

    ntStatus = SamrCopyUnicodeString(&pOut->account_name,
                                     &pIn->account_name,
                                     pDep);
    BAIL_ON_NT_STATUS(ntStatus);

cleanup:
    return ntStatus;

error:
    goto cleanup;
}


static
NTSTATUS
SamrCopyUserInfo8(
    OUT UserInfo8 *pOut,
    IN  UserInfo8 *pIn,
    IN  PVOID      pDep
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;

    BAIL_ON_INVALID_PTR(pOut, ntStatus);
    BAIL_ON_INVALID_PTR(pIn, ntStatus);

    ntStatus = SamrCopyUnicodeString(&pOut->full_name,
                                     &pIn->full_name,
                                     pDep);
    BAIL_ON_NT_STATUS(ntStatus);

cleanup:
    return ntStatus;

error:
    goto cleanup;
}


static
NTSTATUS
SamrCopyUserInfo9(
    OUT UserInfo9 *pOut,
    IN  UserInfo9 *pIn,
    IN  PVOID      pDep
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;

    BAIL_ON_INVALID_PTR(pOut, ntStatus);
    BAIL_ON_INVALID_PTR(pIn, ntStatus);

    pOut->primary_gid = pIn->primary_gid;

cleanup:
    return ntStatus;

error:
    goto cleanup;
}


static
NTSTATUS
SamrCopyUserInfo10(
    OUT UserInfo10 *pOut,
    IN  UserInfo10 *pIn,
    IN  PVOID       pDep
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;

    BAIL_ON_INVALID_PTR(pOut, ntStatus);
    BAIL_ON_INVALID_PTR(pIn, ntStatus);

    ntStatus = SamrCopyUnicodeString(&pOut->home_directory,
                                     &pIn->home_directory,
                                     pDep);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SamrCopyUnicodeString(&pOut->home_drive,
                                     &pIn->home_drive,
                                     pDep);
    BAIL_ON_NT_STATUS(ntStatus);

cleanup:
    return ntStatus;

error:
    goto cleanup;
}


static
NTSTATUS
SamrCopyUserInfo11(
    OUT UserInfo11 *pOut,
    IN  UserInfo11 *pIn,
    IN  PVOID       pDep
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;

    BAIL_ON_INVALID_PTR(pOut, ntStatus);
    BAIL_ON_INVALID_PTR(pIn, ntStatus);

    ntStatus = SamrCopyUnicodeString(&pOut->logon_script,
                                     &pIn->logon_script,
                                     pDep);
    BAIL_ON_NT_STATUS(ntStatus);

cleanup:
    return ntStatus;

error:
    goto cleanup;
}


static
NTSTATUS
SamrCopyUserInfo12(
    OUT UserInfo12 *pOut,
    IN  UserInfo12 *pIn,
    IN  PVOID       pDep
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;

    BAIL_ON_INVALID_PTR(pOut, ntStatus);
    BAIL_ON_INVALID_PTR(pIn, ntStatus);

    ntStatus = SamrCopyUnicodeString(&pOut->profile_path,
                                     &pIn->profile_path,
                                     pDep);
    BAIL_ON_NT_STATUS(ntStatus);

cleanup:
    return ntStatus;

error:
    goto cleanup;
}


static
NTSTATUS
SamrCopyUserInfo13(
    OUT UserInfo13 *pOut,
    IN  UserInfo13 *pIn,
    IN  PVOID       pDep
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;

    BAIL_ON_INVALID_PTR(pOut, ntStatus);
    BAIL_ON_INVALID_PTR(pIn, ntStatus);

    ntStatus = SamrCopyUnicodeString(&pOut->description,
                                     &pIn->description,
                                     pDep);
    BAIL_ON_NT_STATUS(ntStatus);

cleanup:
    return ntStatus;

error:
    goto cleanup;
}


static
NTSTATUS
SamrCopyUserInfo14(
    OUT UserInfo14 *pOut,
    IN  UserInfo14 *pIn,
    IN  PVOID       pDep
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;

    BAIL_ON_INVALID_PTR(pOut, ntStatus);
    BAIL_ON_INVALID_PTR(pIn, ntStatus);

    ntStatus = SamrCopyUnicodeString(&pOut->workstations,
                                     &pIn->workstations,
                                     pDep);
    BAIL_ON_NT_STATUS(ntStatus);

cleanup:
    return ntStatus;

error:
    goto cleanup;
}


static
NTSTATUS
SamrCopyUserInfo16(
    OUT UserInfo16 *pOut,
    IN  UserInfo16 *pIn,
    IN  PVOID       pDep
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;

    BAIL_ON_INVALID_PTR(pOut, ntStatus);
    BAIL_ON_INVALID_PTR(pIn, ntStatus);

    pOut->account_flags = pIn->account_flags;

cleanup:
    return ntStatus;

error:
    goto cleanup;
}


static
NTSTATUS
SamrCopyUserInfo17(
    OUT UserInfo17 *pOut,
    IN  UserInfo17 *pIn,
    IN  PVOID       pDep
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;

    BAIL_ON_INVALID_PTR(pOut, ntStatus);
    BAIL_ON_INVALID_PTR(pIn, ntStatus);

    pOut->account_expiry = pIn->account_expiry;

cleanup:
    return ntStatus;

error:
    goto cleanup;
}


static
NTSTATUS
SamrCopyUserInfo20(
    OUT UserInfo20 *pOut,
    IN  UserInfo20 *pIn,
    IN  PVOID       pDep
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;

    BAIL_ON_INVALID_PTR(pOut, ntStatus);
    BAIL_ON_INVALID_PTR(pIn, ntStatus);

    ntStatus = SamrCopyUnicodeString(&pOut->parameters,
                                     &pIn->parameters,
                                     pDep);
    BAIL_ON_NT_STATUS(ntStatus);

cleanup:
    return ntStatus;

error:
    goto cleanup;
}


static
NTSTATUS
SamrCopyUserInfo21(
    OUT UserInfo21 *pOut,
    IN  UserInfo21 *pIn,
    IN  PVOID       pDep
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;

    BAIL_ON_INVALID_PTR(pOut, ntStatus);
    BAIL_ON_INVALID_PTR(pIn, ntStatus);

    pOut->last_logon            = pIn->last_logon;
    pOut->last_logoff           = pIn->last_logoff;
    pOut->last_password_change  = pIn->last_password_change;
    pOut->account_expiry        = pIn->account_expiry;
    pOut->allow_password_change = pIn->allow_password_change;
    pOut->force_password_change = pIn->force_password_change;

    ntStatus = SamrCopyUnicodeString(&pOut->account_name,
                                     &pIn->account_name,
                                     pDep);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SamrCopyUnicodeString(&pOut->full_name,
                                     &pIn->full_name,
                                     pDep);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SamrCopyUnicodeString(&pOut->home_directory,
                                     &pIn->home_directory,
                                     pDep);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SamrCopyUnicodeString(&pOut->home_drive,
                                     &pIn->home_drive,
                                     pDep);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SamrCopyUnicodeString(&pOut->logon_script,
                                     &pIn->logon_script,
                                     pDep);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SamrCopyUnicodeString(&pOut->profile_path,
                                     &pIn->profile_path,
                                     pDep);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SamrCopyUnicodeString(&pOut->description,
                                     &pIn->description,
                                     pDep);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SamrCopyUnicodeString(&pOut->workstations,
                                     &pIn->workstations,
                                     pDep);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SamrCopyUnicodeString(&pOut->comment,
                                     &pIn->comment,
                                     pDep);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SamrCopyUnicodeString(&pOut->parameters,
                                     &pIn->parameters,
                                     pDep);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SamrCopyUnicodeString(&pOut->unknown1,
                                     &pIn->unknown1,
                                     pDep);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SamrCopyUnicodeString(&pOut->unknown2,
                                     &pIn->unknown2,
                                     pDep);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SamrCopyUnicodeString(&pOut->unknown3,
                                     &pIn->unknown2,
                                     pDep);
    BAIL_ON_NT_STATUS(ntStatus);

    pOut->buf_count = pIn->buf_count;

    ntStatus = SamrAllocateMemory((void**)&pOut->buffer,
                                  pOut->buf_count,
                                  pDep);
    BAIL_ON_NT_STATUS(ntStatus);

    memcpy((void*)pOut->buffer, (void*)pIn->buffer, pOut->buf_count);

    pOut->rid            = pIn->rid;
    pOut->primary_gid    = pIn->primary_gid;
    pOut->account_flags  = pIn->account_flags;
    pOut->fields_present = pIn->fields_present;

    ntStatus = SamrCopyLogonHours(&pOut->logon_hours,
                                  &pIn->logon_hours,
                                  pDep);
    BAIL_ON_NT_STATUS(ntStatus);

    pOut->bad_password_count = pIn->bad_password_count;
    pOut->logon_count        = pIn->logon_count;
    pOut->country_code       = pIn->country_code;
    pOut->code_page          = pIn->code_page;
    pOut->nt_password_set    = pIn->nt_password_set;
    pOut->lm_password_set    = pIn->lm_password_set;
    pOut->password_expired   = pIn->password_expired;
    pOut->unknown4           = pIn->unknown4;

cleanup:
    return ntStatus;

error:
    goto cleanup;
}


static
NTSTATUS
SamrCopyCryptPassword(
    OUT CryptPassword *pOut,
    IN  CryptPassword *pIn,
    IN  PVOID          pDep
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;

    BAIL_ON_INVALID_PTR(pOut, ntStatus);
    BAIL_ON_INVALID_PTR(pIn, ntStatus);

    memcpy((void*)pOut->data, (void*)pIn->data, sizeof(pOut->data));

cleanup:
    return ntStatus;

error:
    goto cleanup;
}


static
NTSTATUS
SamrCopyUserInfo23(
    OUT UserInfo23 *pOut,
    IN  UserInfo23 *pIn,
    IN  PVOID       pDep
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;

    BAIL_ON_INVALID_PTR(pOut, ntStatus);
    BAIL_ON_INVALID_PTR(pIn, ntStatus);

    ntStatus = SamrCopyUserInfo21(&pOut->info,
                                  &pIn->info,
                                  pDep);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SamrCopyCryptPassword(&pOut->password,
                                     &pIn->password,
                                     pDep);
    BAIL_ON_NT_STATUS(ntStatus);

cleanup:
    return ntStatus;

error:
    goto cleanup;
}


static
NTSTATUS
SamrCopyUserInfo24(
    OUT UserInfo24 *pOut,
    IN  UserInfo24 *pIn,
    IN  PVOID       pDep
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;

    BAIL_ON_INVALID_PTR(pOut, ntStatus);
    BAIL_ON_INVALID_PTR(pIn, ntStatus);

    ntStatus = SamrCopyCryptPassword(&pOut->password,
                                     &pIn->password,
                                     pDep);
    BAIL_ON_NT_STATUS(ntStatus);

    pOut->password_len = pIn->password_len;

cleanup:
    return ntStatus;

error:
    goto cleanup;
}


static
NTSTATUS
SamrCopyCryptPasswordEx(
    OUT CryptPasswordEx *pOut,
    IN  CryptPasswordEx *pIn,
    IN  PVOID            pDep
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;

    BAIL_ON_INVALID_PTR(pOut, ntStatus);
    BAIL_ON_INVALID_PTR(pIn, ntStatus);

    memcpy((void*)&pOut->data, (void*)&pIn->data, sizeof(pOut->data));

cleanup:
    return ntStatus;

error:
    goto cleanup;
}


static
NTSTATUS
SamrCopyUserInfo25(
    OUT UserInfo25 *pOut,
    IN  UserInfo25 *pIn,
    IN  PVOID       pDep
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;

    BAIL_ON_INVALID_PTR(pOut, ntStatus);
    BAIL_ON_INVALID_PTR(pIn, ntStatus);

    ntStatus = SamrCopyUserInfo21(&pOut->info,
                                  &pIn->info,
                                  pDep);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SamrCopyCryptPasswordEx(&pOut->password,
                                       &pIn->password,
                                       pDep);
    BAIL_ON_NT_STATUS(ntStatus);

cleanup:
    return ntStatus;

error:
    goto cleanup;
}


static
NTSTATUS
SamrCopyUserInfo26(
    OUT UserInfo26 *pOut,
    IN  UserInfo26 *pIn,
    IN  PVOID       pDep
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;

    BAIL_ON_INVALID_PTR(pOut, ntStatus);
    BAIL_ON_INVALID_PTR(pIn, ntStatus);

    ntStatus = SamrCopyCryptPasswordEx(&pOut->password,
                                       &pIn->password,
                                       pDep);
    BAIL_ON_NT_STATUS(ntStatus);

    pOut->password_len = pIn->password_len;

cleanup:
    return ntStatus;

error:
    goto cleanup;
}


NTSTATUS
SamrAllocateUserInfo(
    OUT UserInfo **ppOut,
    IN  UserInfo *pIn,
    IN  UINT16    Level
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    UserInfo *pInfo = NULL;

    BAIL_ON_INVALID_PTR(ppOut, ntStatus);
    BAIL_ON_INVALID_PTR(pIn, ntStatus);

    ntStatus = SamrAllocateMemory((void*)&pInfo,
                                  sizeof(UserInfo),
                                  NULL);
    BAIL_ON_NT_STATUS(ntStatus);

    if (pIn != NULL) {
        switch (Level) {
        case 1:
            ntStatus = SamrCopyUserInfo1(&pInfo->info1,
                                         &pIn->info1,
                                         (void*)pInfo);
            break;
            
        case 2:
            ntStatus = SamrCopyUserInfo2(&pInfo->info2,
                                         &pIn->info2,
                                         (void*)pInfo);
            break;

        case 3:
            ntStatus = SamrCopyUserInfo3(&pInfo->info3,
                                         &pIn->info3,
                                         (void*)pInfo);
            break;

        case 4:
            ntStatus = SamrCopyUserInfo4(&pInfo->info4,
                                         &pIn->info4,
                                         (void*)pInfo);
            break;

        case 5:
            ntStatus = SamrCopyUserInfo5(&pInfo->info5,
                                         &pIn->info5,
                                         (void*)pInfo);
            break;

        case 6:
            ntStatus = SamrCopyUserInfo6(&pInfo->info6,
                                         &pIn->info6,
                                         (void*)pInfo);
            break;

        case 7:
            ntStatus = SamrCopyUserInfo7(&pInfo->info7,
                                         &pIn->info7,
                                         (void*)pInfo);
            break;

        case 8:
            ntStatus = SamrCopyUserInfo8(&pInfo->info8,
                                         &pIn->info8,
                                         (void*)pInfo);
            break;

        case 9:
            ntStatus = SamrCopyUserInfo9(&pInfo->info9,
                                         &pIn->info9,
                                         (void*)pInfo);
            break;

        case 10:
            ntStatus = SamrCopyUserInfo10(&pInfo->info10,
                                          &pIn->info10,
                                          (void*)pInfo);
            break;

        case 11:
            ntStatus = SamrCopyUserInfo11(&pInfo->info11,
                                          &pIn->info11,
                                          (void*)pInfo);
            break;

        case 12:
            ntStatus = SamrCopyUserInfo12(&pInfo->info12,
                                          &pIn->info12,
                                          (void*)pInfo);
            break;

        case 13:
            ntStatus = SamrCopyUserInfo13(&pInfo->info13,
                                          &pIn->info13,
                                          (void*)pInfo);
            break;

        case 14:
            ntStatus = SamrCopyUserInfo14(&pInfo->info14,
                                          &pIn->info14,
                                          (void*)pInfo);
            break;

        case 16:
            ntStatus = SamrCopyUserInfo16(&pInfo->info16,
                                          &pIn->info16,
                                          (void*)pInfo);
            break;

        case 17:
            ntStatus = SamrCopyUserInfo17(&pInfo->info17,
                                          &pIn->info17,
                                          (void*)pInfo);
            break;

        case 20:
            ntStatus = SamrCopyUserInfo20(&pInfo->info20,
                                          &pIn->info20,
                                          (void*)pInfo);
            break;

        case 21:
            ntStatus = SamrCopyUserInfo21(&pInfo->info21,
                                          &pIn->info21,
                                          (void*)pInfo);
            break;

        case 23:
            ntStatus = SamrCopyUserInfo23(&pInfo->info23,
                                          &pIn->info23,
                                          (void*)pInfo);
            break;

        case 24:
            ntStatus = SamrCopyUserInfo24(&pInfo->info24,
                                          &pIn->info24,
                                          (void*)pInfo);
            break;

        case 25:
            ntStatus = SamrCopyUserInfo25(&pInfo->info25,
                                          &pIn->info25,
                                          (void*)pInfo);
            break;

        case 26:
            ntStatus = SamrCopyUserInfo26(&pInfo->info26,
                                          &pIn->info26,
                                          (void*)pInfo);
            break;

        default:
            ntStatus = STATUS_INVALID_LEVEL;
        }

        BAIL_ON_NT_STATUS(ntStatus);
    }

    *ppOut = pInfo;

cleanup:
    return ntStatus;

error:
    if (pInfo) {
        SamrFreeMemory((void*)pInfo);
    }

    *ppOut = NULL;

    goto cleanup;
}


static
NTSTATUS
SamrCopyDisplayEntryFull(
    OUT SamrDisplayEntryFull *pOut,
    IN  SamrDisplayEntryFull *pIn,
    IN  PVOID                 pDep
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;

    BAIL_ON_INVALID_PTR(pOut, ntStatus);
    BAIL_ON_INVALID_PTR(pIn, ntStatus);

    pOut->idx           = pIn->idx;
    pOut->rid           = pIn->rid;
    pOut->account_flags = pIn->account_flags;

    ntStatus = SamrCopyUnicodeString(&pOut->account_name,
                                     &pIn->account_name,
                                     pDep);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SamrCopyUnicodeString(&pOut->description,
                                     &pIn->description,
                                     pDep);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SamrCopyUnicodeString(&pOut->full_name,
                                     &pIn->full_name,
                                     pDep);
    BAIL_ON_NT_STATUS(ntStatus);

cleanup:
    return ntStatus;

error:
    goto cleanup;
}


static
NTSTATUS
SamrCopyDisplayInfoFull(
    OUT SamrDisplayInfoFull *pOut,
    IN  SamrDisplayInfoFull *pIn,
    IN  PVOID                pDep
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    UINT32 i = 0;

    BAIL_ON_INVALID_PTR(pOut, ntStatus);
    BAIL_ON_INVALID_PTR(pIn, ntStatus);

    pOut->count = pIn->count;

    ntStatus = SamrAllocateMemory((void**)&pOut->entries,
                                  sizeof(pOut->entries[0]) * pOut->count,
                                  pDep);
    BAIL_ON_NT_STATUS(ntStatus);

    for (i = 0; i < pOut->count; i++) {
        ntStatus = SamrCopyDisplayEntryFull(&(pOut->entries[i]),
                                            &(pIn->entries[i]),
                                            pOut->entries);
        BAIL_ON_NT_STATUS(ntStatus);
    }

cleanup:
    return ntStatus;

error:
    goto cleanup;
}


static
NTSTATUS
SamrCopyDisplayEntryGeneral(
    OUT SamrDisplayEntryGeneral *pOut,
    IN  SamrDisplayEntryGeneral *pIn,
    IN  PVOID                    pDep
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;

    BAIL_ON_INVALID_PTR(pOut, ntStatus);
    BAIL_ON_INVALID_PTR(pIn, ntStatus);

    pOut->idx           = pIn->idx;
    pOut->rid           = pIn->rid;
    pOut->account_flags = pIn->account_flags;

    ntStatus = SamrCopyUnicodeString(&pOut->account_name,
                                     &pIn->account_name,
                                     pDep);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SamrCopyUnicodeString(&pOut->description,
                                     &pIn->description,
                                     pDep);
    BAIL_ON_NT_STATUS(ntStatus);

cleanup:
    return ntStatus;

error:
    goto cleanup;
}


static
NTSTATUS
SamrCopyDisplayInfoGeneral(
    OUT SamrDisplayInfoGeneral *pOut,
    IN  SamrDisplayInfoGeneral *pIn,
    IN  PVOID                   pDep
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    uint32 i = 0;

    BAIL_ON_INVALID_PTR(pOut, ntStatus);
    BAIL_ON_INVALID_PTR(pIn, ntStatus);

    pOut->count = pIn->count;

    ntStatus = SamrAllocateMemory((void**)&pOut->entries,
                                  sizeof(pOut->entries[0]) * pOut->count,
                                  pDep);
    BAIL_ON_NT_STATUS(ntStatus);

    for (i = 0; i < pOut->count; i++) {
        ntStatus = SamrCopyDisplayEntryGeneral(&(pOut->entries[i]),
                                               &(pIn->entries[i]),
                                               pOut->entries);
        BAIL_ON_NT_STATUS(ntStatus);
    }

cleanup:
    return ntStatus;

error:
    goto cleanup;
}


static
NTSTATUS
SamrCopyDisplayEntryGeneralGroup(
    OUT SamrDisplayEntryGeneralGroup *pOut,
    IN  SamrDisplayEntryGeneralGroup *pIn,
    IN  PVOID                         pDep
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;

    BAIL_ON_INVALID_PTR(pOut, ntStatus);
    BAIL_ON_INVALID_PTR(pIn, ntStatus);

    pOut->idx           = pIn->idx;
    pOut->rid           = pIn->rid;
    pOut->account_flags = pIn->account_flags;

    ntStatus = SamrCopyUnicodeString(&pOut->account_name,
                                     &pIn->account_name,
                                     pDep);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SamrCopyUnicodeString(&pOut->description,
                                     &pIn->description,
                                     pDep);
    BAIL_ON_NT_STATUS(ntStatus);

cleanup:
    return ntStatus;

error:
    goto cleanup;
}


static
NTSTATUS
SamrCopyDisplayInfoGeneralGroups(
    OUT SamrDisplayInfoGeneralGroups *pOut,
    IN  SamrDisplayInfoGeneralGroups *pIn,
    IN  PVOID                         pDep
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    uint32 i = 0;

    BAIL_ON_INVALID_PTR(pOut, ntStatus);
    BAIL_ON_INVALID_PTR(pIn, ntStatus);

    pOut->count = pIn->count;

    ntStatus = SamrAllocateMemory((void**)&pOut->entries,
                                  sizeof(pOut->entries[0]) * pOut->count,
                                  pDep);
    BAIL_ON_NT_STATUS(ntStatus);

    for (i = 0; i < pOut->count; i++) {
        ntStatus = SamrCopyDisplayEntryGeneralGroup(&(pOut->entries[i]),
                                                    &(pIn->entries[i]),
                                                    pOut->entries);
        BAIL_ON_NT_STATUS(ntStatus);
    }

cleanup:
    return ntStatus;

error:
    goto cleanup;
}


static
NTSTATUS
SamrCopyDisplayEntryAscii(
    OUT SamrDisplayEntryAscii *pOut,
    IN  SamrDisplayEntryAscii *pIn,
    IN  PVOID                  pDep
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;

    BAIL_ON_INVALID_PTR(pOut, ntStatus);
    BAIL_ON_INVALID_PTR(pIn, ntStatus);

    pOut->idx                        = pIn->idx;
    pOut->account_name.Length        = pIn->account_name.Length;
    pOut->account_name.MaximumLength = pIn->account_name.MaximumLength;

    ntStatus = SamrAllocateMemory((void**)&pOut->account_name.Buffer,
                                  sizeof(CHAR) * pOut->account_name.MaximumLength,
                                  pDep);
    BAIL_ON_NT_STATUS(ntStatus);

    memcpy(pOut->account_name.Buffer,
           pIn->account_name.Buffer,
           pOut->account_name.Length);

cleanup:
    return ntStatus;

error:
    goto cleanup;
}


static
NTSTATUS
SamrCopyDisplayInfoAscii(
    OUT SamrDisplayInfoAscii *pOut,
    IN  SamrDisplayInfoAscii *pIn,
    IN  PVOID                 pDep
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    uint32 i = 0;

    BAIL_ON_INVALID_PTR(pOut, ntStatus);
    BAIL_ON_INVALID_PTR(pIn, ntStatus);

    pOut->count = pIn->count;

    ntStatus = SamrAllocateMemory((void**)&pOut->entries,
                                  sizeof(pOut->entries[0]) * pOut->count,
                                  pDep);
    BAIL_ON_NT_STATUS(ntStatus);

    for (i = 0; i < pOut->count; i++) {
        ntStatus = SamrCopyDisplayEntryAscii(&(pOut->entries[i]),
                                             &(pIn->entries[i]),
                                             pOut->entries);
        BAIL_ON_NT_STATUS(ntStatus);
    }

cleanup:
    return ntStatus;

error:
    goto cleanup;
}


NTSTATUS
SamrAllocateDisplayInfo(
    OUT SamrDisplayInfo **ppOut,
    IN  SamrDisplayInfo  *pIn,
    IN  UINT16            Level
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    SamrDisplayInfo *pInfo = NULL;

    BAIL_ON_INVALID_PTR(ppOut, ntStatus);

    ntStatus = SamrAllocateMemory((void**)&pInfo,
                                  sizeof(*pInfo),
                                  NULL);
    BAIL_ON_NT_STATUS(ntStatus);

    if (pIn == NULL) goto cleanup;

    switch (Level) {
    case 1:
        ntStatus = SamrCopyDisplayInfoFull(&pInfo->info1,
                                           &pIn->info1,
                                           pInfo);
        break;

    case 2:
        ntStatus = SamrCopyDisplayInfoGeneral(&pInfo->info2,
                                              &pIn->info2,
                                              pInfo);
        break;

    case 3:
        ntStatus = SamrCopyDisplayInfoGeneralGroups(&pInfo->info3,
                                                    &pIn->info3,
                                                    pInfo);
        break;

    case 4:
        ntStatus = SamrCopyDisplayInfoAscii(&pInfo->info4,
                                            &pIn->info4,
                                            pInfo);
        break;

    case 5:
        ntStatus = SamrCopyDisplayInfoAscii(&pInfo->info5,
                                            &pIn->info5,
                                            pInfo);

    default:
        ntStatus = STATUS_INVALID_INFO_CLASS;
    }

    BAIL_ON_NT_STATUS(ntStatus);

    *ppOut = pInfo;

cleanup:
    return ntStatus;

error:
    if (pInfo) {
        SamrFreeMemory((void*)pInfo);
    }

    *ppOut = NULL;

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
