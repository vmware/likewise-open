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
    CHAR szFilterFmt[] = "(&(objectclass=group)(gid=%d))";
    NTSTATUS status = STATUS_SUCCESS;
    DWORD dwError = 0;
    PACCOUNT_CONTEXT pAccCtx = NULL;
    PDOMAIN_CONTEXT pDomCtx = NULL;
    PCONNECT_CONTEXT pConnCtx = NULL;
    PWSTR pwszBase = NULL;
    DWORD dwScope = 0;
    PWSTR pwszFilter = NULL;
    DWORD dwFilterLen = 0;
    PWSTR wszAttributes[] = { NULL };
    PDIRECTORY_ENTRY pEntry = NULL;
    DWORD dwEntriesNum = 0;
    AliasInfo *pAliasInfo = NULL;

    pAccCtx = (PACCOUNT_CONTEXT)hAlias;

    if (pAccCtx == NULL || pAccCtx->Type == SamrContextAccount) {
        status = STATUS_INVALID_HANDLE;
        BAIL_ON_NTSTATUS_ERROR(status);
    }

    pDomCtx  = pAccCtx->pDomCtx;
    pConnCtx = pDomCtx->pConnCtx;

    pwszBase = pDomCtx->pwszDn;

    dwFilterLen = sizeof(szFilterFmt) + 10;
    status = SamrSrvAllocateMemory((void**)&pwszFilter,
                                   dwFilterLen * sizeof(WCHAR),
                                   NULL);
    BAIL_ON_NTSTATUS_ERROR(status);

    sw16printf(pwszFilter, szFilterFmt, pAccCtx->dwRid);

    dwError = DirectorySearch(pConnCtx->hDirectory,
                              pwszBase,
                              dwScope,
                              pwszFilter,
                              wszAttributes,
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

    status = SamrSrvAllocateMemory((void**)&pAliasInfo,
                                   sizeof(*pAliasInfo),
                                   pAccCtx);
    BAIL_ON_NTSTATUS_ERROR(status);

    switch (level) {
    case ALIAS_INFO_ALL:
        status = SamrFillAliasInfo1(pEntry, pAliasInfo);
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
    AliasInfo *pInfo
    )
{
    WCHAR wszEmptyString[] = { (WCHAR)'\0' };
    NTSTATUS status = STATUS_SUCCESS;
    DWORD dwError = 0;
    AliasInfoAll *pInfoAll = NULL;
    PDIRECTORY_ATTRIBUTE pAttr = NULL;
    PATTRIBUTE_VALUE pAttrVal = NULL;
    PWSTR pwszName = NULL;
    PWSTR pwszDescription = NULL;
    DWORD dwNumMembers = 0;

    pInfoAll = &(pInfo->all);

    /* name */
    dwError = DirectoryGetEntryAttrValueByNameA(pEntry,
                                                DIRECTORY_ATTR_TAG_GROUP_NAME,
                                                DIRECTORY_ATTR_TYPE_UNICODE_STRING,
                                                &pwszName);
    BAIL_ON_LSA_ERROR(dwError);

    status = InitUnicodeString(&pInfoAll->name, pwszName);
    BAIL_ON_NTSTATUS_ERROR(status);

    status = SamrSrvAddDepMemory(pInfoAll->name.string, pInfo);
    BAIL_ON_NTSTATUS_ERROR(status);


    /* num_members */
    pInfoAll->num_members = 0;

    /* description */
    status = InitUnicodeString(&pInfoAll->description, wszEmptyString);
    BAIL_ON_NTSTATUS_ERROR(status);

    status = SamrSrvAddDepMemory(pInfoAll->description.string, pInfo);
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
    WCHAR wszEmptyString[] = { (WCHAR)'\0' };
    NTSTATUS status = STATUS_SUCCESS;
    DWORD dwError = 0;
    PDIRECTORY_ATTRIBUTE pAttr = NULL;
    PATTRIBUTE_VALUE pAttrVal = NULL;
    PWSTR pwszName = NULL;

    /* name */
    dwError = DirectoryGetEntryAttrValueByNameA(pEntry,
                                                DIRECTORY_ATTR_TAG_GROUP_NAME,
                                                DIRECTORY_ATTR_TYPE_UNICODE_STRING,
                                                &pwszName);
    BAIL_ON_LSA_ERROR(dwError);

    status = InitUnicodeString(&pInfo->name, pwszName);
    BAIL_ON_NTSTATUS_ERROR(status);

    status = SamrSrvAddDepMemory(pInfo->name.string, pInfo);
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
    WCHAR wszEmptyString[] = { (WCHAR)'\0' };
    NTSTATUS status = STATUS_SUCCESS;
    DWORD dwError = 0;
    PDIRECTORY_ATTRIBUTE pAttr = NULL;
    PATTRIBUTE_VALUE pAttrVal = NULL;
    PWSTR pwszDescription = NULL;

    /* name */
    status = InitUnicodeString(&pInfo->description, wszEmptyString);
    BAIL_ON_NTSTATUS_ERROR(status);

    status = SamrSrvAddDepMemory(pInfo->description.string, pInfo);
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
