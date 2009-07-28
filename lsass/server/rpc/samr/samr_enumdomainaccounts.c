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
 *        samr_enumdomainaccounts.c
 *
 * Abstract:
 *
 *        Remote Procedure Call (RPC) Server Interface
 *
 *        SamrEnumDomainAccounts function
 *
 * Authors: Rafal Szczesniak (rafal@likewise.com)
 */

#include "includes.h"


NTSTATUS
SamrSrvEnumDomainAccounts(
    handle_t hBinding,
    DOMAIN_HANDLE hDomain,
    uint32 *resume,
    DWORD dwObjectClass,
    uint32 account_flags,
    uint32 max_size,
    RidNameArray **names,
    uint32 *num_entries
    )
{
    wchar_t wszFilterFmt[] = L"%ws=%d AND %ws='%ws'";
    NTSTATUS status = STATUS_SUCCESS;
    DWORD dwError = 0;
    PDOMAIN_CONTEXT pDomCtx = NULL;
    PCONNECT_CONTEXT pConnCtx = NULL;
    PWSTR pwszBase = NULL;
    WCHAR wszAttrObjectClass[] = DS_ATTR_OBJECT_CLASS;
    WCHAR wszAttrDomainName[] = DS_ATTR_DOMAIN;
    WCHAR wszAttrAccountFlags[] = DS_ATTR_ACCOUNT_FLAGS;
    WCHAR wszAttrSamAccountName[] = DS_ATTR_SAM_ACCOUNT_NAME;
    WCHAR wszAttrObjectSid[] = DS_ATTR_OBJECT_SID;
    DWORD dwScope = 0;
    PWSTR pwszFilter = NULL;
    PWSTR pwszDomainName = NULL;
    DWORD dwFilterLen = 0;
    PWSTR wszAttributes[4];
    PDIRECTORY_ENTRY pEntries = NULL;
    PDIRECTORY_ENTRY pEntry = NULL;
    DWORD dwEntriesNum = 0;
    DWORD dwSize = 0;
    DWORD i = 0;
    DWORD j = 0;
    DWORD dwCount = 0;
    DWORD dwResume = 0;
    DWORD dwAccountFlags = 0;
    PWSTR pwszName = NULL;
    PWSTR pwszSid = NULL;
    PSID pSid = NULL;
    DWORD dwRid = 0;
    RidNameArray *pNames = NULL;
    RidName *pName = NULL;

    pDomCtx = (PDOMAIN_CONTEXT)hDomain;

    if (pDomCtx == NULL || pDomCtx->Type != SamrContextDomain) {
        status = STATUS_INVALID_HANDLE;
        BAIL_ON_NTSTATUS_ERROR(status);
    }

    pConnCtx       = pDomCtx->pConnCtx;
    pwszBase       = pDomCtx->pwszDn;
    pwszDomainName = pDomCtx->pwszDomainName;

    dwFilterLen = ((sizeof(wszAttrObjectClass)/sizeof(WCHAR)) - 1) +
                  10 +
                  ((sizeof(wszAttrDomainName)/sizeof(WCHAR)) - 1) +
                  (wc16slen(pwszDomainName) + 1) +
                  (sizeof(wszFilterFmt)/sizeof(wszFilterFmt[0]));

    status = SamrSrvAllocateMemory((void**)&pwszFilter,
                                   dwFilterLen * sizeof(*pwszFilter));
    BAIL_ON_NTSTATUS_ERROR(status);

    sw16printfw(pwszFilter, dwFilterLen, wszFilterFmt,
                wszAttrObjectClass,
                dwObjectClass,
                wszAttrDomainName,
                pwszDomainName);

    wszAttributes[0] = wszAttrSamAccountName;
    wszAttributes[1] = wszAttrObjectSid;
    wszAttributes[2] = wszAttrAccountFlags;
    wszAttributes[3] = NULL;

    dwError = DirectorySearch(pConnCtx->hDirectory,
                              pwszBase,
                              dwScope,
                              pwszFilter,
                              wszAttributes,
                              FALSE,
                              &pEntries,
                              &dwEntriesNum);
    BAIL_ON_LSA_ERROR(dwError);

    status = SamrSrvAllocateMemory((void**)&pNames,
                                   sizeof(RidNameArray));
    BAIL_ON_NTSTATUS_ERROR(status);

    i = (*resume);

    if (i >= dwEntriesNum) {
        i = 0;
        status = STATUS_NO_MORE_ENTRIES;
        BAIL_ON_NTSTATUS_ERROR(status);
    }

    dwSize += sizeof(pNames->count);
    for (; dwSize < max_size && i < dwEntriesNum; i++) {
        pEntry = &(pEntries[i]);

        dwError = DirectoryGetEntryAttrValueByName(pEntry,
                                                   wszAttrSamAccountName,
                                                   DIRECTORY_ATTR_TYPE_UNICODE_STRING,
                                                   &pwszName);
        BAIL_ON_LSA_ERROR(dwError);

        dwError = DirectoryGetEntryAttrValueByName(pEntry,
                                                   wszAttrAccountFlags,
                                                   DIRECTORY_ATTR_TYPE_INTEGER,
                                                   &dwAccountFlags);
        BAIL_ON_LSA_ERROR(dwError);

        if (!account_flags ||
            (dwAccountFlags & account_flags)) {
            dwSize += sizeof(uint32);
            dwSize += wc16slen(pwszName) * sizeof(wchar16_t);
            dwSize += 2 * sizeof(uint16);

            dwCount++;
        }
    }

    /* At least one domain entry is returned regardless of declared
       max response size */
    dwCount  = (!dwCount) ? 1 : dwCount;
    dwResume = (*resume) + dwCount;

    status = SamrSrvAllocateMemory((void**)&pNames->entries,
                                   sizeof(pNames->entries[0]) * dwCount);
    BAIL_ON_NTSTATUS_ERROR(status);

    dwCount = 0;
    i       = (*resume);

    for (j = 0; i < dwResume && j < dwEntriesNum; j++)
    {
        pEntry = &(pEntries[j]);

        dwError = DirectoryGetEntryAttrValueByName(pEntry,
                                                   wszAttrObjectSid,
                                                   DIRECTORY_ATTR_TYPE_UNICODE_STRING,
                                                   &pwszSid);
        BAIL_ON_LSA_ERROR(dwError);

        dwError = DirectoryGetEntryAttrValueByName(pEntry,
                                                   wszAttrSamAccountName,
                                                   DIRECTORY_ATTR_TYPE_UNICODE_STRING,
                                                   &pwszName);
        BAIL_ON_LSA_ERROR(dwError);

        dwError = DirectoryGetEntryAttrValueByName(pEntry,
                                                   wszAttrAccountFlags,
                                                   DIRECTORY_ATTR_TYPE_INTEGER,
                                                   &dwAccountFlags);
        BAIL_ON_LSA_ERROR(dwError);

        if (account_flags &&
            !(dwAccountFlags & account_flags)) continue;

        pName = &(pNames->entries[i - (*resume)]);

        status = SamrSrvAllocateSidFromWC16String(&pSid, pwszSid);
        BAIL_ON_NTSTATUS_ERROR(status);

        dwRid = pSid->SubAuthority[pSid->SubAuthorityCount - 1];

        pName->rid = (uint32)dwRid;

        status = SamrSrvInitUnicodeString(&pName->name,
                                          pwszName);
        BAIL_ON_NTSTATUS_ERROR(status);

        if (pSid) {
            SamrSrvFreeMemory(pSid);
            pSid = NULL;
        }

        i++;
        dwCount++;
    }

    pNames->count = dwCount;

    *resume       = dwResume;
    *num_entries  = dwCount;
    *names        = pNames;

cleanup:
    if (pwszFilter) {
        SamrSrvFreeMemory(pwszFilter);
    }

    if (pSid) {
        SamrSrvFreeMemory(pSid);
    }

    if (pEntries) {
        DirectoryFreeEntries(pEntries, dwEntriesNum);
    }

    return status;

error:
    if (pNames) {
        SamrSrvFreeMemory(pNames);
    }

    *resume      = 0;
    *num_entries = 0;
    *names       = NULL;
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
