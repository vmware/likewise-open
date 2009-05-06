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


static
NTSTATUS
SamrSrvFillDisplayInfoGeneralGroups(
    PDOMAIN_CONTEXT pDomCtx,
    PDIRECTORY_ENTRY pEntry,
    SamrDisplayInfo *pInfo,
    DWORD i,
    DWORD dwCount,
    PDWORD pdwSize
    );


static
NTSTATUS
SamrSrvFillDisplayInfoAscii(
    PDOMAIN_CONTEXT pDomCtx,
    PDIRECTORY_ENTRY pEntry,
    SamrDisplayInfoAscii *pInfo,
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
    const wchar_t wszFilterFmt[] = L"%ws=%d AND %ws>%d";

    WCHAR wszAttrObjectClass[] = DS_ATTR_OBJECT_CLASS;
    WCHAR wszAttrRecordId[] = DS_ATTR_RECORD_ID;
    WCHAR wszAttrObjectSid[] = DS_ATTR_OBJECT_SID;
    WCHAR wszAttrSamAccountName[] = DS_ATTR_SAM_ACCOUNT_NAME;
    WCHAR wszAttrDescription[] = DS_ATTR_DESCRIPTION;
    WCHAR wszAttrFullName[] = DS_ATTR_FULL_NAME;

    PWSTR wszAttributesLevel1[] = {
        wszAttrRecordId,
        wszAttrObjectSid,
        wszAttrSamAccountName,
        wszAttrDescription,
        wszAttrFullName,
        NULL
    };

    PWSTR wszAttributesLevel2[] = {
        wszAttrRecordId,
        wszAttrObjectSid,
        wszAttrSamAccountName,
        wszAttrDescription,
        NULL
    };

    PWSTR wszAttributesLevel3[] = {
        wszAttrRecordId,
        wszAttrObjectSid,
        wszAttrSamAccountName,
        wszAttrDescription,
        NULL
    };

    PWSTR wszAttributesLevel4[] = {
        wszAttrRecordId,
        wszAttrSamAccountName,
        NULL
    };

    PWSTR wszAttributesLevel5[] = {
        wszAttrRecordId,
        wszAttrSamAccountName,
        NULL
    };

    PWSTR *pwszAttributes[] = {
        wszAttributesLevel1,
        wszAttributesLevel2,
        wszAttributesLevel3,
        wszAttributesLevel4,
        wszAttributesLevel5
    };

    NTSTATUS status = STATUS_SUCCESS;
    DWORD dwError = 0;
    PDOMAIN_CONTEXT pDomCtx = NULL;
    PCONNECT_CONTEXT pConnCtx = NULL;
    PWSTR pwszBase = NULL;
    DWORD dwScope = 0;
    DWORD dwObjectClass = 0;
    PWSTR pwszFilter = NULL;
    DWORD dwFilterLen = 0;
    PDIRECTORY_ENTRY pEntries = NULL;
    PDIRECTORY_ENTRY pEntry = NULL;
    DWORD dwEntriesNum = 0;
    SamrDisplayInfo Info;
    DWORD dwTotalSize = 0;
    DWORD dwSize = 0;
    DWORD dwCount = 0;
    DWORD i = 0;

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
    case 4:
    case 5:
        dwObjectClass = DS_OBJECT_CLASS_USER;
        break;

    case 3:
        dwObjectClass = DS_OBJECT_CLASS_GROUP;
        break;

    default:
        status = STATUS_INVALID_LEVEL;
        BAIL_ON_NTSTATUS_ERROR(status);
        break;
    }

    dwFilterLen = ((sizeof(wszAttrObjectClass)/sizeof(WCHAR)) - 1) +
                  10 +
                  ((sizeof(wszAttrRecordId)/sizeof(WCHAR)) - 1) +
                  10 +
                  (sizeof(wszFilterFmt)/sizeof(wszFilterFmt[0]));

    status = SamrSrvAllocateMemory((void**)&pwszFilter,
                                   dwFilterLen * sizeof(*pwszFilter),
                                   pDomCtx);
    BAIL_ON_NTSTATUS_ERROR(status);

    sw16printfw(pwszFilter, dwFilterLen, wszFilterFmt,
                wszAttrObjectClass,
                dwObjectClass,
                wszAttrRecordId,
                start_idx);

    dwError = DirectorySearch(pConnCtx->hDirectory,
                              pwszBase,
                              dwScope,
                              pwszFilter,
                              pwszAttributes[level - 1],
                              FALSE,
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

        case 3:
            status = SamrSrvFillDisplayInfoGeneralGroups(pDomCtx,
                                                         pEntry,
                                                         NULL,
                                                         i,
                                                         dwCount,
                                                         &dwTotalSize);
            break;

        case 4:
        case 5:
            status = SamrSrvFillDisplayInfoAscii(pDomCtx,
                                                 pEntry,
                                                 NULL,
                                                 i,
                                                 dwCount,
                                                 &dwTotalSize);
            break;
        }

        BAIL_ON_NTSTATUS_ERROR(status);

        if (dwTotalSize < buf_size && i < max_entries) {
            dwCount = i + 1;
        }
    }

    /* At least one account entry is returned regardless of declared
       max response size */
    dwCount  = (!dwCount) ? 1 : dwCount;

    dwSize += sizeof(uint32);    /* "count" field in info structure */
    i       = start_idx;

    if (dwEntriesNum == 0) {
        i = 0;
        status = STATUS_NO_MORE_ENTRIES;
        BAIL_ON_NTSTATUS_ERROR(status);
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

        case 3:
            status = SamrSrvFillDisplayInfoGeneralGroups(pDomCtx,
                                                         pEntry,
                                                         &Info,
                                                         i,
                                                         dwCount,
                                                         &dwSize);
            break;

        case 4:
            status = SamrSrvFillDisplayInfoAscii(pDomCtx,
                                                 pEntry,
                                                 &Info.info4,
                                                 i,
                                                 dwCount,
                                                 &dwSize);
            break;

        case 5:
            status = SamrSrvFillDisplayInfoAscii(pDomCtx,
                                                 pEntry,
                                                 &Info.info5,
                                                 i,
                                                 dwCount,
                                                 &dwSize);
            break;
        }

        BAIL_ON_NTSTATUS_ERROR(status);
    }

    if (dwCount < dwEntriesNum) {
        status = STATUS_MORE_ENTRIES;
    }

    *total_size    = dwTotalSize;
    *returned_size = dwSize;
    *info          = Info;

cleanup:
    if (pwszFilter) {
        SamrSrvFreeMemory(pwszFilter);
    }

    if (pEntries) {
        DirectoryFreeEntries(pEntries, dwEntriesNum);
    }

    return status;

error:
    /* Regardless of info level the pointer is at the same position */
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
    WCHAR wszAttrRecordId[] = DS_ATTR_RECORD_ID;
    WCHAR wszAttrObjectSid[] = DS_ATTR_OBJECT_SID;
    WCHAR wszAttrSamAccountName[] = DS_ATTR_SAM_ACCOUNT_NAME;
    WCHAR wszAttrDescription[] = DS_ATTR_DESCRIPTION;
    WCHAR wszAttrFullName[] = DS_ATTR_FULL_NAME;
    PWSTR pwszSid = NULL;
    PWSTR pwszUsername = NULL;
    PWSTR pwszDescription = NULL;
    PWSTR pwszFullName = NULL;
    LONG64 llRecId = 0;
    PSID pSid = NULL;
    DWORD dwRid = 0;
    ULONG ulAccountFlags = 0;
    SamrDisplayInfoFull *pInfo1 = NULL;
    SamrDisplayEntryFull *pDisplayEntry = NULL;
    DWORD dwSize = 0;

    dwError = DirectoryGetEntryAttrValueByName(pEntry,
                                               wszAttrRecordId,
                                               DIRECTORY_ATTR_TYPE_LARGE_INTEGER,
                                               &llRecId);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = DirectoryGetEntryAttrValueByName(pEntry,
                                               wszAttrObjectSid,
                                               DIRECTORY_ATTR_TYPE_UNICODE_STRING,
                                               &pwszSid);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = DirectoryGetEntryAttrValueByName(pEntry,
                                               wszAttrSamAccountName,
                                               DIRECTORY_ATTR_TYPE_UNICODE_STRING,
                                               &pwszUsername);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = DirectoryGetEntryAttrValueByName(pEntry,
                                               wszAttrDescription,
                                               DIRECTORY_ATTR_TYPE_UNICODE_STRING,
                                               &pwszDescription);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = DirectoryGetEntryAttrValueByName(pEntry,
                                               wszAttrFullName,
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

    pDisplayEntry->idx           = (uint32)llRecId;

    status = SamrSrvAllocateSidFromWC16String(&pSid, pwszSid,
                                              pInfo1->entries);
    BAIL_ON_NTSTATUS_ERROR(status);

    dwRid = pSid->SubAuthority[pSid->SubAuthorityCount - 1];

    pDisplayEntry->rid           = (uint32)dwRid;
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
                                      pwszDescription,
                                      pInfo1->entries);
    BAIL_ON_NTSTATUS_ERROR(status);

done:
    *pdwSize  = dwSize;

cleanup:
    if (pSid) {
        SamrSrvFreeMemory(pSid);
    }

    return status;

error:
    if (pInfo1 && pInfo1->entries) {
        SamrSrvFreeMemory(pInfo1->entries);
        pInfo1->entries = NULL;
    }

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
    WCHAR wszAttrRecordId[] = DS_ATTR_RECORD_ID;
    WCHAR wszAttrObjectSid[] = DS_ATTR_OBJECT_SID;
    WCHAR wszAttrSamAccountName[] = DS_ATTR_SAM_ACCOUNT_NAME;
    WCHAR wszAttrDescription[] = DS_ATTR_DESCRIPTION;
    PWSTR pwszSid = NULL;
    PWSTR pwszUsername = NULL;
    PWSTR pwszDescription = NULL;
    LONG64 llRecId = 0;
    PSID pSid = NULL;
    ULONG dwRid = 0;
    ULONG ulAccountFlags = 0;
    SamrDisplayInfoGeneral *pInfo2 = NULL;
    SamrDisplayEntryGeneral *pDisplayEntry = NULL;
    DWORD dwSize = 0;

    dwError = DirectoryGetEntryAttrValueByName(pEntry,
                                               wszAttrRecordId,
                                               DIRECTORY_ATTR_TYPE_LARGE_INTEGER,
                                               &llRecId);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = DirectoryGetEntryAttrValueByName(pEntry,
                                               wszAttrObjectSid,
                                               DIRECTORY_ATTR_TYPE_UNICODE_STRING,
                                               &pwszSid);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = DirectoryGetEntryAttrValueByName(pEntry,
                                               wszAttrSamAccountName,
                                               DIRECTORY_ATTR_TYPE_UNICODE_STRING,
                                               &pwszUsername);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = DirectoryGetEntryAttrValueByName(pEntry,
                                               wszAttrDescription,
                                               DIRECTORY_ATTR_TYPE_UNICODE_STRING,
                                               &pwszDescription);
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

    pDisplayEntry->idx           = (uint32)llRecId;

    status = SamrSrvAllocateSidFromWC16String(&pSid, pwszSid,
                                              pInfo2->entries);
    BAIL_ON_NTSTATUS_ERROR(status);

    dwRid = pSid->SubAuthority[pSid->SubAuthorityCount - 1];

    pDisplayEntry->rid           = (uint32)dwRid;
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
    if (pSid) {
        SamrSrvFreeMemory(pSid);
    }

    return status;

error:
    if (pInfo2 && pInfo2->entries) {
        SamrSrvFreeMemory(pInfo2->entries);
        pInfo2->entries = NULL;
    }

    goto cleanup;
}


static
NTSTATUS
SamrSrvFillDisplayInfoGeneralGroups(
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
    WCHAR wszAttrRecordId[] = DS_ATTR_RECORD_ID;
    WCHAR wszAttrObjectSid[] = DS_ATTR_OBJECT_SID;
    WCHAR wszAttrSamAccountName[] = DS_ATTR_SAM_ACCOUNT_NAME;
    WCHAR wszAttrDescription[] = DS_ATTR_DESCRIPTION;
    PWSTR pwszSid = NULL;
    PWSTR pwszUsername = NULL;
    PWSTR pwszDescription = NULL;
    LONG64 llRecId = 0;
    PSID pSid = NULL;
    ULONG dwRid = 0;
    ULONG ulAccountFlags = 0;
    SamrDisplayInfoGeneralGroups *pInfo3 = NULL;
    SamrDisplayEntryGeneralGroup *pDisplayEntry = NULL;
    DWORD dwSize = 0;

    dwError = DirectoryGetEntryAttrValueByName(pEntry,
                                               wszAttrRecordId,
                                               DIRECTORY_ATTR_TYPE_LARGE_INTEGER,
                                               &llRecId);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = DirectoryGetEntryAttrValueByName(pEntry,
                                               wszAttrObjectSid,
                                               DIRECTORY_ATTR_TYPE_UNICODE_STRING,
                                               &pwszSid);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = DirectoryGetEntryAttrValueByName(pEntry,
                                               wszAttrSamAccountName,
                                               DIRECTORY_ATTR_TYPE_UNICODE_STRING,
                                               &pwszUsername);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = DirectoryGetEntryAttrValueByName(pEntry,
                                               wszAttrDescription,
                                               DIRECTORY_ATTR_TYPE_UNICODE_STRING,
                                               &pwszDescription);
    BAIL_ON_LSA_ERROR(dwError);

    dwSize  = (*pdwSize);
    dwSize += sizeof(pInfo->info3.entries[0]);
    dwSize += wc16slen(pwszUsername) * sizeof(WCHAR);

    /* If NULL pointer is passed we're just calculating the size */
    if (pInfo == NULL) goto done;
    pInfo3 = &pInfo->info3;

    if (!pInfo3->entries) {
        status = SamrSrvAllocateMemory((void**)&pInfo3->entries,
                                       sizeof(pInfo3->entries[0])
                                       * dwCount,
                                       pDomCtx);
        pInfo3->count = dwCount;
    }

    pDisplayEntry = &(pInfo3->entries[i]);

    pDisplayEntry->idx           = (uint32)llRecId;

    status = SamrSrvAllocateSidFromWC16String(&pSid, pwszSid,
                                              pInfo3->entries);
    BAIL_ON_NTSTATUS_ERROR(status);

    dwRid = pSid->SubAuthority[pSid->SubAuthorityCount - 1];

    pDisplayEntry->rid           = (uint32)dwRid;
    pDisplayEntry->account_flags = (uint32)ulAccountFlags;

    status = SamrSrvInitUnicodeString(&pDisplayEntry->account_name,
                                      pwszUsername,
                                      pInfo3->entries);
    BAIL_ON_NTSTATUS_ERROR(status);

    status = SamrSrvInitUnicodeString(&pDisplayEntry->description,
                                      NULL,
                                      pInfo3->entries);
    BAIL_ON_NTSTATUS_ERROR(status);

done:
    *pdwSize  = dwSize;

cleanup:
    if (pSid) {
        SamrSrvFreeMemory(pSid);
    }

    return status;

error:
    if (pInfo3 && pInfo3->entries) {
        SamrSrvFreeMemory(pInfo3->entries);
        pInfo3->entries = NULL;
    }

    goto cleanup;
}


static
NTSTATUS
SamrSrvFillDisplayInfoAscii(
    PDOMAIN_CONTEXT pDomCtx,
    PDIRECTORY_ENTRY pEntry,
    SamrDisplayInfoAscii *pInfo,
    DWORD i,
    DWORD dwCount,
    PDWORD pdwSize
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    DWORD dwError = 0;
    WCHAR wszAttrRecordId[] = DS_ATTR_RECORD_ID;
    WCHAR wszAttrSamAccountName[] = DS_ATTR_SAM_ACCOUNT_NAME;
    PWSTR pwszUsername = NULL;
    LONG64 llRecId = 0;
    SamrDisplayEntryAscii *pDisplayEntry = NULL;
    DWORD dwSize = 0;
    DWORD dwUsernameLen = 0;

    dwError = DirectoryGetEntryAttrValueByName(pEntry,
                                               wszAttrRecordId,
                                               DIRECTORY_ATTR_TYPE_LARGE_INTEGER,
                                               &llRecId);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = DirectoryGetEntryAttrValueByName(pEntry,
                                               wszAttrSamAccountName,
                                               DIRECTORY_ATTR_TYPE_UNICODE_STRING,
                                               &pwszUsername);
    BAIL_ON_LSA_ERROR(dwError);

    dwUsernameLen = wc16slen(pwszUsername);

    dwSize  = (*pdwSize);
    dwSize += sizeof(pInfo->entries[0]);
    dwSize += (dwUsernameLen + 1) * sizeof(CHAR);

    /* If NULL pointer is passed we're just calculating the size */
    if (pInfo == NULL) goto done;

    if (!pInfo->entries) {
        status = SamrSrvAllocateMemory((void**)&pInfo->entries,
                                       sizeof(pInfo->entries[0])
                                       * dwCount,
                                       pDomCtx);
        pInfo->count = dwCount;
    }

    pDisplayEntry = &(pInfo->entries[i]);

    pDisplayEntry->idx = (uint32)llRecId;

    pDisplayEntry->account_name.Length        = dwUsernameLen;
    pDisplayEntry->account_name.MaximumLength = dwUsernameLen;

    status = SamrSrvAllocateMemory((void**)&pDisplayEntry->account_name.Buffer,
                                   (dwUsernameLen + 1) * sizeof(CHAR),
                                   pInfo->entries);
    BAIL_ON_NTSTATUS_ERROR(status);

    wc16stombs(pDisplayEntry->account_name.Buffer,
               pwszUsername,
               dwUsernameLen + 1);

done:
    *pdwSize  = dwSize;

cleanup:
    return status;

error:
    if (pInfo && pInfo->entries) {
        SamrSrvFreeMemory(pInfo->entries);
        pInfo->entries = NULL;
    }

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
