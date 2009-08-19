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
 *        samr_queryaliasinfo.c
 *
 * Abstract:
 *
 *        Remote Procedure Call (RPC) Server Interface
 *
 *        SamrQueryAliasInfo function
 *
 * Authors: Rafal Szczesniak (rafal@likewise.com)
 */

#include "includes.h"


static
NTSTATUS
SamrFillAliasInfo1(
    PDIRECTORY_ENTRY pEntry,
    DWORD dwNumMembers,
    AliasInfo *pInfo
    );


static
NTSTATUS
SamrFillAliasInfo2(
    PDIRECTORY_ENTRY pEntry,
    AliasInfo *pInfo
    );


static
NTSTATUS
SamrFillAliasInfo3(
    PDIRECTORY_ENTRY pEntry,
    AliasInfo *pInfo
    );


NTSTATUS
SamrSrvQueryAliasInfo(
    /* [in] */ handle_t hBinding,
    /* [in] */ ACCOUNT_HANDLE hAlias,
    /* [in] */ uint16 level,
    /* [out] */ AliasInfo **info
    )
{
    wchar_t wszFilterFmt[] = L"%ws='%ws'";
    NTSTATUS status = STATUS_SUCCESS;
    DWORD dwError = 0;
    PACCOUNT_CONTEXT pAcctCtx = NULL;
    PDOMAIN_CONTEXT pDomCtx = NULL;
    PCONNECT_CONTEXT pConnCtx = NULL;
    PWSTR pwszBase = NULL;
    WCHAR wszAttrDn[] = DS_ATTR_DISTINGUISHED_NAME;
    WCHAR wszAttrSamAccountName[] = DS_ATTR_SAM_ACCOUNT_NAME;
    WCHAR wszAttrDescription[] = DS_ATTR_DESCRIPTION;
    DWORD dwScope = 0;
    PWSTR pwszFilter = NULL;
    DWORD dwFilterLen = 0;
    PDIRECTORY_ENTRY pEntry = NULL;
    DWORD dwEntriesNum = 0;
    PDIRECTORY_ENTRY pMemberEntry = NULL;
    DWORD dwNumMembers = 0;
    AliasInfo *pAliasInfo = NULL;

    PWSTR wszAttributesLevel1[] = {
        wszAttrSamAccountName,
        wszAttrDescription,
        NULL
    };

    PWSTR wszAttributesLevel2[] = {
        wszAttrSamAccountName,
        NULL
    };

    PWSTR wszAttributesLevel3[] = {
        wszAttrDescription,
        NULL
    };

    PWSTR *pwszAttributes[] = {
        wszAttributesLevel1,
        wszAttributesLevel2,
        wszAttributesLevel3
    };

    PWSTR wszMemberAttributes[] = {
        wszAttrDn,
        NULL
    };

    pAcctCtx = (PACCOUNT_CONTEXT)hAlias;

    if (pAcctCtx == NULL || pAcctCtx->Type != SamrContextAccount) {
        status = STATUS_INVALID_HANDLE;
        BAIL_ON_NTSTATUS_ERROR(status);
    }

    pDomCtx  = pAcctCtx->pDomCtx;
    pConnCtx = pDomCtx->pConnCtx;

    pwszBase = pDomCtx->pwszDn;

    dwFilterLen = ((sizeof(wszAttrDn)/sizeof(WCHAR)) - 1) +
                  wc16slen(pAcctCtx->pwszDn) +
                  (sizeof(wszFilterFmt)/sizeof(wszFilterFmt[0]));

    status = SamrSrvAllocateMemory((void**)&pwszFilter,
                                   dwFilterLen * sizeof(WCHAR));
    BAIL_ON_NTSTATUS_ERROR(status);

    sw16printfw(pwszFilter, dwFilterLen, wszFilterFmt,
                wszAttrDn, pAcctCtx->pwszDn);

    dwError = DirectorySearch(pConnCtx->hDirectory,
                              pwszBase,
                              dwScope,
                              pwszFilter,
                              pwszAttributes[level - 1],
                              FALSE,
                              &pEntry,
                              &dwEntriesNum);
    BAIL_ON_LSA_ERROR(dwError);

    if (dwEntriesNum == 0) {
        status = STATUS_INVALID_HANDLE;

    } else if (dwEntriesNum > 1) {
        status = STATUS_INTERNAL_ERROR;
    }

    BAIL_ON_NTSTATUS_ERROR(status);

    if (level == ALIAS_INFO_ALL) {
        dwError = DirectoryGetGroupMembers(pConnCtx->hDirectory,
                                           pAcctCtx->pwszDn,
                                           wszMemberAttributes,
                                           &pMemberEntry,
                                           &dwNumMembers);
        BAIL_ON_LSA_ERROR(dwError);
    }

    status = SamrSrvAllocateMemory((void**)&pAliasInfo,
                                   sizeof(*pAliasInfo));
    BAIL_ON_NTSTATUS_ERROR(status);

    switch (level) {
    case ALIAS_INFO_ALL:
        status = SamrFillAliasInfo1(pEntry, dwNumMembers, pAliasInfo);
        break;

    case ALIAS_INFO_NAME:
        status = SamrFillAliasInfo2(pEntry, pAliasInfo);
        break;

    case ALIAS_INFO_DESCRIPTION:
        status = SamrFillAliasInfo3(pEntry, pAliasInfo);
        break;

    default:
        status = STATUS_INVALID_INFO_CLASS;
    }

    BAIL_ON_NTSTATUS_ERROR(status);

    *info = pAliasInfo;

cleanup:
    if (pwszFilter) {
        SamrSrvFreeMemory(pwszFilter);
    }

    if (pEntry) {
        DirectoryFreeEntries(pEntry, dwEntriesNum);
    }

    return status;

error:
    if (pAliasInfo) {
        SamrSrvFreeMemory(pAliasInfo);
    }

    *info = NULL;
    goto cleanup;
}


static
NTSTATUS
SamrFillAliasInfo1(
    PDIRECTORY_ENTRY pEntry,
    DWORD dwNumMembers,
    AliasInfo *pInfo
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    DWORD dwError = 0;
    AliasInfoAll *pInfoAll = NULL;
    WCHAR wszAttrSamAccountName[] = DS_ATTR_SAM_ACCOUNT_NAME;
    WCHAR wszAttrDescription[] = DS_ATTR_DESCRIPTION;
    PWSTR pwszName = NULL;
    PWSTR pwszDescription = NULL;

    pInfoAll = &(pInfo->all);

    /* name */
    dwError = DirectoryGetEntryAttrValueByName(pEntry,
                                               wszAttrSamAccountName,
                                               DIRECTORY_ATTR_TYPE_UNICODE_STRING,
                                               &pwszName);
    BAIL_ON_LSA_ERROR(dwError);

    status = SamrSrvInitUnicodeString(&pInfoAll->name, pwszName);
    BAIL_ON_NTSTATUS_ERROR(status);

    /* num_members */
    pInfoAll->num_members = dwNumMembers;

    /* description */
    dwError = DirectoryGetEntryAttrValueByName(pEntry,
                                               wszAttrDescription,
                                               DIRECTORY_ATTR_TYPE_UNICODE_STRING,
                                               &pwszDescription);
    BAIL_ON_LSA_ERROR(dwError);

    status = SamrSrvInitUnicodeString(&pInfoAll->description, pwszDescription);
    BAIL_ON_NTSTATUS_ERROR(status);

cleanup:
    return status;

error:
    memset(pInfo, 0, sizeof(*pInfo));
    goto cleanup;
}


static
NTSTATUS
SamrFillAliasInfo2(
    PDIRECTORY_ENTRY pEntry,
    AliasInfo *pInfo
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    DWORD dwError = 0;
    WCHAR wszAttrSamrAccountName[] = DS_ATTR_SAM_ACCOUNT_NAME;
    PWSTR pwszName = NULL;

    /* name */
    dwError = DirectoryGetEntryAttrValueByName(pEntry,
                                               wszAttrSamrAccountName,
                                               DIRECTORY_ATTR_TYPE_UNICODE_STRING,
                                               &pwszName);
    BAIL_ON_LSA_ERROR(dwError);

    status = SamrSrvInitUnicodeString(&pInfo->name, pwszName);
    BAIL_ON_NTSTATUS_ERROR(status);

cleanup:
    return status;

error:
    memset(pInfo, 0, sizeof(*pInfo));
    goto cleanup;
}


static
NTSTATUS
SamrFillAliasInfo3(
    PDIRECTORY_ENTRY pEntry,
    AliasInfo *pInfo
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    DWORD dwError = 0;
    WCHAR wszAttrDescription[] = DS_ATTR_DESCRIPTION;
    PWSTR pwszDescription = NULL;

    /* name */
    dwError = DirectoryGetEntryAttrValueByName(pEntry,
                                               wszAttrDescription,
                                               DIRECTORY_ATTR_TYPE_UNICODE_STRING,
                                               &pwszDescription);
    BAIL_ON_LSA_ERROR(dwError);

    status = SamrSrvInitUnicodeString(&pInfo->description, pwszDescription);
    BAIL_ON_NTSTATUS_ERROR(status);

cleanup:
    return status;

error:
    memset(pInfo, 0, sizeof(*pInfo));
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
