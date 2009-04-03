/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 */

/*
 * Copyright Likewise Software    2004-2009
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
 * Copyright (C) Likewise Software. All rights reserved.
 *
 * Module Name:
 *
 *        samr_querydisplayinfo.c
 *
 * Abstract:
 *
 *        Remote Procedure Call (RPC) Server Interface
 *
 *        SamrQueryDisplayInfo function
 *
 * Authors: Rafal Szczesniak (rafal@likewise.com)
 */

#include "includes.h"


static
NTSTATUS
SamrSrvFillDisplayInfoFull(
    PDOMAIN_CONTEXT pDomCtx,
    PDIRECTORY_ENTRY pEntry,
    SamrDisplayInfo *pInfo,
    DWORD i,
    DWORD dwCount,
    PDWORD pdwSize
    );


static
NTSTATUS
SamrSrvFillDisplayInfoGeneral(
    PDOMAIN_CONTEXT pDomCtx,
    PDIRECTORY_ENTRY pEntry,
    SamrDisplayInfo *pInfo,
    DWORD i,
    DWORD dwCount,
    PDWORD pdwSize
    );


NTSTATUS
SamrSrvQueryDisplayInfo(
    /* [in] */ handle_t IDL_handle,
    /* [in] */ DOMAIN_HANDLE hDomain,
    /* [in] */ uint16 level,
    /* [in] */ uint32 start_idx,
    /* [in] */ uint32 max_entries,
    /* [in] */ uint32 buf_size,
    /* [out] */ uint32 *total_size,
    /* [out] */ uint32 *returned_size,
    /* [out] */ SamrDisplayInfo *info
    )
{
    const CHAR szFilterUsers[] = "(objectclass=user)";
    const CHAR szFilterGroups[] = "(objectclass=group)";
    const CHAR szFilterUsersGroups[] = "(|(objectclass=user)(objectclass=group))";

    NTSTATUS status = STATUS_SUCCESS;
    DWORD dwError = 0;
    PDOMAIN_CONTEXT pDomCtx = NULL;
    PCONNECT_CONTEXT pConnCtx = NULL;
    PWSTR pwszBase = NULL;
    DWORD dwScope = 0;
    PCSTR pszFilter = NULL;
    PWSTR pwszFilter = NULL;
    PWSTR wszAttributes[6];
    PWSTR pwszAttrNameUid = NULL;
    PWSTR pwszAttrNameGid = NULL;
    PWSTR pwszAttrNameUsername = NULL;
    PWSTR pwszAttrNameGroupname = NULL;
    PWSTR pwszAttrNameAccountFlags = NULL;
    PWSTR pwszAttrNameFullName = NULL;
    PWSTR pwszAttrFullName = NULL;
    PDIRECTORY_ENTRY pEntries = NULL;
    PDIRECTORY_ENTRY pEntry = NULL;
    DWORD dwEntriesNum = 0;
    SamrDisplayInfo Info;
    DWORD dwTotalSize = 0;
    DWORD dwSize = 0;
    DWORD dwCount = 0;
    DWORD i = 0;

    memset(wszAttributes, 0, sizeof(wszAttributes));
    memset(&Info, 0, sizeof(Info));

    pDomCtx = (PDOMAIN_CONTEXT)hDomain;

    if (pDomCtx == NULL || pDomCtx->Type != SamrContextDomain) {
        status = STATUS_INVALID_HANDLE;
        BAIL_ON_NTSTATUS_ERROR(status);
    }

    pConnCtx = pDomCtx->pConnCtx;
    pwszBase = pDomCtx->pwszDn;

    switch (level) {
    case 1:
    case 2:
        pszFilter = (PCSTR)szFilterUsers;
        break;

    case 3:
        pszFilter = (PCSTR)szFilterGroups;
        break;

    case 4:
    case 5:
        pszFilter = (PCSTR)szFilterUsersGroups;

    default:
        status = STATUS_INVALID_LEVEL;
        BAIL_ON_NTSTATUS_ERROR(status);
        break;
    }

    dwError = LsaMbsToWc16s(pszFilter, &pwszFilter);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaMbsToWc16s(DIRECTORY_ATTR_TAG_UID,
                            &pwszAttrNameUid);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaMbsToWc16s(DIRECTORY_ATTR_TAG_GID,
                            &pwszAttrNameGid);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaMbsToWc16s(DIRECTORY_ATTR_TAG_USER_NAME,
                            &pwszAttrNameUsername);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaMbsToWc16s(DIRECTORY_ATTR_TAG_GROUP_NAME,
                            &pwszAttrNameGroupname);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaMbsToWc16s(DIRECTORY_ATTR_TAG_USER_INFO_FLAGS,
                            &pwszAttrNameAccountFlags);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaMbsToWc16s(DIRECTORY_ATTR_TAG_USER_FULLNAME,
                            &pwszAttrNameFullName);
    BAIL_ON_LSA_ERROR(dwError);

    switch (level) {
    case 1:
        wszAttributes[0] = pwszAttrNameUid;
        wszAttributes[1] = pwszAttrNameUsername;
        wszAttributes[2] = pwszAttrNameAccountFlags;
        wszAttributes[3] = pwszAttrNameFullName;
        wszAttributes[4] = NULL;
        break;

    case 2:
        wszAttributes[0] = pwszAttrNameUid;
        wszAttributes[1] = pwszAttrNameUsername;
        wszAttributes[2] = pwszAttrNameAccountFlags;
        wszAttributes[3] = NULL;
        break;

    case 3:
        wszAttributes[0] = pwszAttrNameGid;
        wszAttributes[1] = pwszAttrNameGroupname;
        wszAttributes[2] = pwszAttrNameAccountFlags;
        wszAttributes[3] = NULL;
        break;

    case 4:
    case 5:
        wszAttributes[0] = pwszAttrNameUsername;
        wszAttributes[1] = pwszAttrNameGroupname;
        wszAttributes[2] = NULL;
        break;
    }

    dwError = DirectorySearch(pConnCtx->hDirectory,
                              pwszBase,
                              dwScope,
                              pwszFilter,
                              wszAttributes,
                              TRUE,
                              &pEntries,
                              &dwEntriesNum);
    BAIL_ON_LSA_ERROR(dwError);

    dwTotalSize += sizeof(uint32);    /* "count" field in info structure */

    for (i = 0; i < dwEntriesNum; i++) {
        pEntry = &(pEntries[i]);

        switch (level) {
        case 1:
            status = SamrSrvFillDisplayInfoFull(pDomCtx,
                                                pEntry,
                                                NULL,
                                                i,
                                                dwCount,
                                                &dwTotalSize);
            break;

        case 2:
            status = SamrSrvFillDisplayInfoGeneral(pDomCtx,
                                                   pEntry,
                                                   NULL,
                                                   i,
                                                   dwCount,
                                                   &dwTotalSize);
            break;
        }

        BAIL_ON_NTSTATUS_ERROR(status);

        if (dwTotalSize < buf_size && i < max_entries) {
            dwCount = i;
        }
    }

    dwSize += sizeof(uint32);    /* "count" field in info structure */
    i       = start_idx;

    if (i >= dwEntriesNum) {
        i = 0;
        status = STATUS_NO_MORE_ENTRIES;
    }

    for (i = 0; i < dwCount && i < dwEntriesNum; i++) {
        pEntry = &(pEntries[i]);

        switch (level) {
        case 1:
            status = SamrSrvFillDisplayInfoFull(pDomCtx,
                                                pEntry,
                                                &Info,
                                                i,
                                                dwCount,
                                                &dwSize);
            break;

        case 2:
            status = SamrSrvFillDisplayInfoGeneral(pDomCtx,
                                                   pEntry,
                                                   &Info,
                                                   i,
                                                   dwCount,
                                                   &dwSize);
            break;
        }

        BAIL_ON_NTSTATUS_ERROR(status);
    }

    *total_size    = dwTotalSize;
    *returned_size = dwSize;
    *info          = Info;

cleanup:
    if (pwszFilter) {
        LSA_SAFE_FREE_MEMORY(pwszFilter);
    }

    if (pwszAttrNameUid) {
        LSA_SAFE_FREE_MEMORY(pwszAttrNameUid);
    }

    if (pwszAttrNameGid) {
        LSA_SAFE_FREE_MEMORY(pwszAttrNameGid);
    }

    if (pwszAttrNameUsername) {
        LSA_SAFE_FREE_MEMORY(pwszAttrNameUsername);
    }

    if (pwszAttrNameGroupname) {
        LSA_SAFE_FREE_MEMORY(pwszAttrNameGroupname);
    }

    if (pwszAttrNameAccountFlags) {
        LSA_SAFE_FREE_MEMORY(pwszAttrNameAccountFlags);
    }

    if (pwszAttrFullName) {
        LSA_SAFE_FREE_MEMORY(pwszAttrNameFullName);
    }

    return status;

error:
    /* Regardless of info level the pointer is at the same position */
    SamrSrvFreeMemory(Info.info1.entries);
    memset(&Info, 0, sizeof(Info));

    goto cleanup;
}


static
NTSTATUS
SamrSrvFillDisplayInfoFull(
    PDOMAIN_CONTEXT pDomCtx,
    PDIRECTORY_ENTRY pEntry,
    SamrDisplayInfo *pInfo,
    DWORD i,
    DWORD dwCount,
    PDWORD pdwSize
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    DWORD dwError = 0;
    ULONG ulRid = 0;
    ULONG ulAccountFlags = 0;
    PWSTR pwszUsername = NULL;
    PWSTR pwszFullName = NULL;
    SamrDisplayInfoFull *pInfo1 = NULL;
    SamrDisplayEntryFull *pDisplayEntry = NULL;
    DWORD dwSize = 0;

    dwError = DirectoryGetEntryAttrValueByNameA(pEntry,
                                                DIRECTORY_ATTR_TAG_UID,
                                                DIRECTORY_ATTR_TYPE_LARGE_INTEGER,
                                                &ulRid);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = DirectoryGetEntryAttrValueByNameA(pEntry,
                                                DIRECTORY_ATTR_TAG_USER_INFO_FLAGS,
                                                DIRECTORY_ATTR_TYPE_LARGE_INTEGER,
                                                &ulAccountFlags);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = DirectoryGetEntryAttrValueByNameA(pEntry,
                                                DIRECTORY_ATTR_TAG_USER_NAME,
                                                DIRECTORY_ATTR_TYPE_UNICODE_STRING,
                                                &pwszUsername);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = DirectoryGetEntryAttrValueByNameA(pEntry,
                                                DIRECTORY_ATTR_TAG_USER_FULLNAME,
                                                DIRECTORY_ATTR_TYPE_UNICODE_STRING,
                                                &pwszFullName);
    BAIL_ON_LSA_ERROR(dwError);

    dwSize  = (*pdwSize);
    dwSize += sizeof(pInfo->info1.entries[0]);
    dwSize += wc16slen(pwszUsername) * sizeof(WCHAR);
    dwSize += wc16slen(pwszFullName) * sizeof(WCHAR);

    /* If NULL pointer is passed we're just calculating the size */
    if (pInfo == NULL) goto done;
    pInfo1 = &pInfo->info1;

    if (!pInfo1->entries) {
        status = SamrSrvAllocateMemory((void**)&pInfo1->entries,
                                       sizeof(pInfo1->entries[0])
                                       * dwCount,
                                       pDomCtx);
        pInfo1->count = dwCount;
    }

    pDisplayEntry = &(pInfo1->entries[i]);

    pDisplayEntry->idx           = (uint32)i;
    pDisplayEntry->rid           = (uint32)ulRid;
    pDisplayEntry->account_flags = (uint32)ulAccountFlags;

    status = SamrSrvInitUnicodeString(&pDisplayEntry->account_name,
                                      pwszUsername,
                                      pInfo1->entries);
    BAIL_ON_NTSTATUS_ERROR(status);

    status = SamrSrvInitUnicodeString(&pDisplayEntry->full_name,
                                      pwszFullName,
                                      pInfo1->entries);
    BAIL_ON_NTSTATUS_ERROR(status);

    status = SamrSrvInitUnicodeString(&pDisplayEntry->description,
                                      NULL,
                                      pInfo1->entries);
    BAIL_ON_NTSTATUS_ERROR(status);

done:
    *pdwSize  = dwSize;

cleanup:
    return status;

error:
    goto cleanup;
}


static
NTSTATUS
SamrSrvFillDisplayInfoGeneral(
    PDOMAIN_CONTEXT pDomCtx,
    PDIRECTORY_ENTRY pEntry,
    SamrDisplayInfo *pInfo,
    DWORD i,
    DWORD dwCount,
    PDWORD pdwSize
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    DWORD dwError = 0;
    ULONG ulRid = 0;
    ULONG ulAccountFlags = 0;
    PWSTR pwszUsername = NULL;
    SamrDisplayInfoGeneral *pInfo2 = NULL;
    SamrDisplayEntryGeneral *pDisplayEntry = NULL;
    DWORD dwSize = 0;

    dwError = DirectoryGetEntryAttrValueByNameA(pEntry,
                                                DIRECTORY_ATTR_TAG_UID,
                                                DIRECTORY_ATTR_TYPE_LARGE_INTEGER,
                                                &ulRid);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = DirectoryGetEntryAttrValueByNameA(pEntry,
                                                DIRECTORY_ATTR_TAG_USER_INFO_FLAGS,
                                                DIRECTORY_ATTR_TYPE_LARGE_INTEGER,
                                                &ulAccountFlags);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = DirectoryGetEntryAttrValueByNameA(pEntry,
                                                DIRECTORY_ATTR_TAG_USER_NAME,
                                                DIRECTORY_ATTR_TYPE_UNICODE_STRING,
                                                &pwszUsername);
    BAIL_ON_LSA_ERROR(dwError);

    dwSize  = (*pdwSize);
    dwSize += sizeof(pInfo->info2.entries[0]);
    dwSize += wc16slen(pwszUsername) * sizeof(WCHAR);

    /* If NULL pointer is passed we're just calculating the size */
    if (pInfo == NULL) goto done;
    pInfo2 = &pInfo->info2;

    if (!pInfo2->entries) {
        status = SamrSrvAllocateMemory((void**)&pInfo2->entries,
                                       sizeof(pInfo2->entries[0])
                                       * dwCount,
                                       pDomCtx);
        pInfo2->count = dwCount;
    }

    pDisplayEntry = &(pInfo2->entries[i]);

    pDisplayEntry->idx           = (uint32)i;
    pDisplayEntry->rid           = (uint32)ulRid;
    pDisplayEntry->account_flags = (uint32)ulAccountFlags;

    status = SamrSrvInitUnicodeString(&pDisplayEntry->account_name,
                                      pwszUsername,
                                      pInfo2->entries);
    BAIL_ON_NTSTATUS_ERROR(status);

    status = SamrSrvInitUnicodeString(&pDisplayEntry->description,
                                      NULL,
                                      pInfo2->entries);
    BAIL_ON_NTSTATUS_ERROR(status);

done:
    *pdwSize  = dwSize;

cleanup:
    return status;

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
