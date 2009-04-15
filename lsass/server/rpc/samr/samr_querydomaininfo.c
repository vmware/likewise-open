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
 * Copyright (C) Likewise Software. All rights reserved.
 *
 * Module Name:
 *
 *        samr_querydomaininfo.c
 *
 * Abstract:
 *
 *        Remote Procedure Call (RPC) Server Interface
 *
 *        SamrQueryDomainInfo function
 *
 * Authors: Rafal Szczesniak (rafal@likewise.com)
 */

#include "includes.h"


static
NTSTATUS
SamrSrvFillDomainInfo1(
    PDOMAIN_CONTEXT pDomCtx,
    PDIRECTORY_ENTRY pEntry,
    DomainInfo *pInfo
    );


static
NTSTATUS
SamrSrvFillDomainInfo2(
    PDOMAIN_CONTEXT pDomCtx,
    PDIRECTORY_ENTRY pEntry,
    DomainInfo *pInfo
    );


static
NTSTATUS
SamrSrvFillDomainInfo3(
    PDOMAIN_CONTEXT pDomCtx,
    PDIRECTORY_ENTRY pEntry,
    DomainInfo *pInfo
    );


static
NTSTATUS
SamrSrvFillDomainInfo4(
    PDOMAIN_CONTEXT pDomCtx,
    PDIRECTORY_ENTRY pEntry,
    DomainInfo *pInfo
    );


static
NTSTATUS
SamrSrvFillDomainInfo5(
    PDOMAIN_CONTEXT pDomCtx,
    PDIRECTORY_ENTRY pEntry,
    DomainInfo *pInfo
    );


static
NTSTATUS
SamrSrvFillDomainInfo6(
    PDOMAIN_CONTEXT pDomCtx,
    PDIRECTORY_ENTRY pEntry,
    DomainInfo *pInfo
    );


static
NTSTATUS
SamrSrvFillDomainInfo7(
    PDOMAIN_CONTEXT pDomCtx,
    PDIRECTORY_ENTRY pEntry,
    DomainInfo *pInfo
    );


static
NTSTATUS
SamrSrvFillDomainInfo8(
    PDOMAIN_CONTEXT pDomCtx,
    PDIRECTORY_ENTRY pEntry,
    DomainInfo *pInfo
    );


static
NTSTATUS
SamrSrvFillDomainInfo9(
    PDOMAIN_CONTEXT pDomCtx,
    PDIRECTORY_ENTRY pEntry,
    DomainInfo *pInfo
    );


static
NTSTATUS
SamrSrvFillDomainInfo11(
    PDOMAIN_CONTEXT pDomCtx,
    PDIRECTORY_ENTRY pEntry,
    DomainInfo *pInfo
    );


static
NTSTATUS
SamrSrvFillDomainInfo12(
    PDOMAIN_CONTEXT pDomCtx,
    PDIRECTORY_ENTRY pEntry,
    DomainInfo *pInfo
    );


static
NTSTATUS
SamrSrvFillDomainInfo13(
    PDOMAIN_CONTEXT pDomCtx,
    PDIRECTORY_ENTRY pEntry,
    DomainInfo *pInfo
    );



NTSTATUS
SamrSrvQueryDomainInfo(
    /* [in] */ handle_t hBinding,
    /* [in] */ DOMAIN_HANDLE hDomain,
    /* [in] */ uint16 level,
    /* [out] */ DomainInfo **ppInfo
    )
{
    const wchar_t wszFilterFmt[] = L"(%ws=%d OR %ws=%d) AND %ws='%ws'";

    WCHAR wszAttrObjectClass[] = DS_ATTR_OBJECT_CLASS;
    WCHAR wszAttrDn[] = DS_ATTR_DISTINGUISHED_NAME;
    WCHAR wszAttrDomain[] = DS_ATTR_DOMAIN;
    WCHAR wszAttrComment[] = DS_ATTR_COMMENT;
    WCHAR wszAttrCreatedTime[] = DS_ATTR_CREATED_TIME;

    PWSTR wszAttributeLevel1[] = {
        wszAttrDn,
        NULL
    };

    PWSTR wszAttributeLevel2[] = {
        wszAttrDn,
        wszAttrDomain,
        wszAttrComment,
        NULL
    };

    PWSTR wszAttributeLevel3[] = {
        wszAttrDn,
        NULL
    };

    PWSTR wszAttributeLevel4[] = {
        wszAttrDn,
        wszAttrComment,
        NULL
    };

    PWSTR wszAttributeLevel5[] = {
        wszAttrDn,
        wszAttrDomain,
        NULL
    };

    PWSTR wszAttributeLevel6[] = {
        wszAttrDn,
        NULL
    };

    PWSTR wszAttributeLevel7[] = {
        wszAttrDn,
        NULL
    };

    PWSTR wszAttributeLevel8[] = {
        wszAttrDn,
        wszAttrCreatedTime,
        NULL
    };

    PWSTR wszAttributeLevel9[] = {
        wszAttrDn,
        NULL
    };

    PWSTR wszAttributeLevel10[] = {
        wszAttrDn,
        NULL
    };

    PWSTR wszAttributeLevel11[] = {
        wszAttrDn,
        wszAttrDomain,
        wszAttrComment,
        NULL
    };

    PWSTR wszAttributeLevel12[] = {
        wszAttrDn,
        NULL
    };

    PWSTR wszAttributeLevel13[] = {
        wszAttrDn,
        wszAttrCreatedTime,
        NULL
    };

    PWSTR *pwszAttributes[] = {
        wszAttributeLevel1,
        wszAttributeLevel2,
        wszAttributeLevel3,
        wszAttributeLevel4,
        wszAttributeLevel5,
        wszAttributeLevel6,
        wszAttributeLevel7,
        wszAttributeLevel8,
        wszAttributeLevel9,
        wszAttributeLevel10,
        wszAttributeLevel11,
        wszAttributeLevel12,
        wszAttributeLevel13
    };

    NTSTATUS status = STATUS_SUCCESS;
    DWORD dwError = 0;
    PDOMAIN_CONTEXT pDomCtx = NULL;
    PCONNECT_CONTEXT pConnCtx = NULL;
    PWSTR pwszBase = NULL;
    PWSTR pwszDn = NULL;
    DWORD dwScope = 0;
    DWORD dwObjectClassDomain = DS_OBJECT_CLASS_DOMAIN;
    DWORD dwObjectClassBuiltin = DS_OBJECT_CLASS_BUILTIN_DOMAIN;
    PWSTR pwszFilter = NULL;
    DWORD dwFilterLen = 0;
    PDIRECTORY_ENTRY pEntry = NULL;
    DWORD dwEntriesNum = 0;
    DomainInfo *pInfo = NULL;

    pDomCtx  = (PDOMAIN_CONTEXT)hDomain;
    pConnCtx = pDomCtx->pConnCtx;

    if (pDomCtx == NULL || pDomCtx->Type != SamrContextDomain) {
        status = STATUS_INVALID_HANDLE;
        BAIL_ON_NTSTATUS_ERROR(status);
    }

    pwszDn   = pDomCtx->pwszDn;
    pwszBase = pDomCtx->pwszDn;

    dwFilterLen = ((sizeof(wszAttrObjectClass)/sizeof(WCHAR)) - 1) +
                  10 +
                  ((sizeof(wszAttrObjectClass)/sizeof(WCHAR)) - 1) +
                  10 +
                  ((sizeof(wszAttrDn)/sizeof(WCHAR)) - 1) +
                  wc16slen(pwszDn) +
                  (sizeof(wszFilterFmt)/sizeof(wszFilterFmt[0]));

    status = SamrSrvAllocateMemory((void**)&pwszFilter,
                                   dwFilterLen * sizeof(WCHAR),
                                   pDomCtx);
    BAIL_ON_NTSTATUS_ERROR(status);

    sw16printfw(pwszFilter, dwFilterLen, wszFilterFmt,
                wszAttrObjectClass,
                dwObjectClassDomain,
                wszAttrObjectClass,
                dwObjectClassBuiltin,
                wszAttrDn,
                pwszDn);

    dwError = DirectorySearch(pConnCtx->hDirectory,
                              pwszBase,
                              dwScope,
                              pwszFilter,
                              pwszAttributes[level - 1],
                              FALSE,
                              &pEntry,
                              &dwEntriesNum);
    BAIL_ON_LSA_ERROR(dwError);

    status = SamrSrvAllocateMemory((void**)&pInfo,
                                   sizeof(*pInfo),
                                   pDomCtx);
    BAIL_ON_NTSTATUS_ERROR(status);

    switch (level) {
    case 1:
        status = SamrSrvFillDomainInfo1(pDomCtx,
                                        pEntry,
                                        pInfo);
        break;

    case 2:
        status = SamrSrvFillDomainInfo2(pDomCtx,
                                        pEntry,
                                        pInfo);
        break;

    case 3:
        status = SamrSrvFillDomainInfo3(pDomCtx,
                                        pEntry,
                                        pInfo);
        break;

    case 4:
        status = SamrSrvFillDomainInfo4(pDomCtx,
                                        pEntry,
                                        pInfo);
        break;

    case 5:
        status = SamrSrvFillDomainInfo5(pDomCtx,
                                        pEntry,
                                        pInfo);
        break;

    case 6:
        status = SamrSrvFillDomainInfo6(pDomCtx,
                                        pEntry,
                                        pInfo);
        break;

    case 7:
        status = SamrSrvFillDomainInfo7(pDomCtx,
                                        pEntry,
                                        pInfo);
        break;

    case 8:
        status = SamrSrvFillDomainInfo8(pDomCtx,
                                        pEntry,
                                        pInfo);
        break;

    case 9:
        status = SamrSrvFillDomainInfo9(pDomCtx,
                                        pEntry,
                                        pInfo);
        break;

    case 11:
        status = SamrSrvFillDomainInfo11(pDomCtx,
                                         pEntry,
                                         pInfo);
        break;

    case 12:
        status = SamrSrvFillDomainInfo12(pDomCtx,
                                         pEntry,
                                         pInfo);
        break;

    case 13:
        status = SamrSrvFillDomainInfo13(pDomCtx,
                                         pEntry,
                                         pInfo);
        break;

    default:
        status = STATUS_INVALID_INFO_CLASS;
        break;
    }

    BAIL_ON_NTSTATUS_ERROR(status);

    *ppInfo = pInfo;

cleanup:
    if (pwszFilter) {
        SamrSrvFreeMemory(pwszFilter);
    }

    if (pEntry) {
        DirectoryFreeEntries(pEntry, dwEntriesNum);
    }

    return status;

error:
    if (pInfo) {
        SamrSrvFreeMemory(pInfo);
    }

    *ppInfo = NULL;
    goto cleanup;
}


static
NTSTATUS
SamrSrvFillDomainInfo1(
    PDOMAIN_CONTEXT pDomCtx,
    PDIRECTORY_ENTRY pEntry,
    DomainInfo *pInfo
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    DomainInfo1 *pInfo1 = NULL;

    pInfo1 = &pInfo->info1;

    pInfo1->min_pass_length = 0;
    pInfo1->pass_history_length = 5;
    pInfo1->pass_properties = 0;
    pInfo1->max_pass_age    = 0;
    pInfo1->min_pass_age    = 0;

    return status;
}


static
NTSTATUS
SamrSrvFillDomainInfo2(
    PDOMAIN_CONTEXT pDomCtx,
    PDIRECTORY_ENTRY pEntry,
    DomainInfo *pInfo
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    DWORD dwError = 0;
    WCHAR wszAttrComment[] = DS_ATTR_COMMENT;
    WCHAR wszAttrDomainName[] = DS_ATTR_DOMAIN;
    PWSTR pwszComment = NULL;
    PWSTR pwszDomainName = NULL;
    DomainInfo2 *pInfo2 = NULL;

    dwError = DirectoryGetEntryAttrValueByName(pEntry,
                                               wszAttrComment,
                                               DIRECTORY_ATTR_TYPE_UNICODE_STRING,
                                               &pwszComment);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = DirectoryGetEntryAttrValueByName(pEntry,
                                               wszAttrDomainName,
                                               DIRECTORY_ATTR_TYPE_UNICODE_STRING,
                                               &pwszDomainName);
    BAIL_ON_LSA_ERROR(dwError);

    pInfo2 = &pInfo->info2;

    /* force_logoff_time */
    pInfo2->force_logoff_time = 0;

    /* comment */
    status = SamrSrvInitUnicodeString(&pInfo2->comment,
                                      pwszComment,
                                      pInfo);
    BAIL_ON_NTSTATUS_ERROR(status);

    /* domain_name */
    status = SamrSrvInitUnicodeString(&pInfo2->domain_name,
                                      pwszDomainName,
                                      pInfo);
    BAIL_ON_NTSTATUS_ERROR(status);

    /* primary */
    status = SamrSrvInitUnicodeString(&pInfo2->primary, NULL,
                                      pInfo);
    BAIL_ON_NTSTATUS_ERROR(status);

    /* sequence_num */
    /* unknown1 */
    /* role */
    pInfo2->role = SAMR_ROLE_DOMAIN_MEMBER;

    /* unknown2 */
    /* num_users */
    /* num_groups */
    /* num_aliases */

cleanup:
    return status;

error:
    goto cleanup;
}


static
NTSTATUS
SamrSrvFillDomainInfo3(
    PDOMAIN_CONTEXT pDomCtx,
    PDIRECTORY_ENTRY pEntry,
    DomainInfo *pInfo
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    DomainInfo3 *pInfo3 = NULL;

    pInfo3 = &pInfo->info3;
    pInfo3->force_logoff_time = 0;

    return status;
}


static
NTSTATUS
SamrSrvFillDomainInfo4(
    PDOMAIN_CONTEXT pDomCtx,
    PDIRECTORY_ENTRY pEntry,
    DomainInfo *pInfo
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    DWORD dwError = 0;
    WCHAR wszAttrComment[] = DS_ATTR_COMMENT;
    PWSTR pwszComment = NULL;
    DomainInfo4 *pInfo4 = NULL;

    dwError = DirectoryGetEntryAttrValueByName(pEntry,
                                               wszAttrComment,
                                               DIRECTORY_ATTR_TYPE_UNICODE_STRING,
                                               &pwszComment);
    BAIL_ON_LSA_ERROR(dwError);

    pInfo4 = &pInfo->info4;

    status = SamrSrvInitUnicodeString(&pInfo4->comment,
                                      pwszComment,
                                      pInfo);
    BAIL_ON_NTSTATUS_ERROR(status);

cleanup:
    return status;

error:
    goto cleanup;
}


static
NTSTATUS
SamrSrvFillDomainInfo5(
    PDOMAIN_CONTEXT pDomCtx,
    PDIRECTORY_ENTRY pEntry,
    DomainInfo *pInfo
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    DWORD dwError = 0;
    WCHAR wszAttrDomainName[] = DS_ATTR_DOMAIN;
    PWSTR pwszDomainName = NULL;
    DomainInfo5 *pInfo5 = NULL;

    dwError = DirectoryGetEntryAttrValueByName(pEntry,
                                               wszAttrDomainName,
                                               DIRECTORY_ATTR_TYPE_UNICODE_STRING,
                                               &pwszDomainName);
    BAIL_ON_LSA_ERROR(dwError);

    pInfo5 = &pInfo->info5;

    status = SamrSrvInitUnicodeString(&pInfo5->domain_name,
                                      pwszDomainName,
                                      pInfo);
    BAIL_ON_NTSTATUS_ERROR(status);

cleanup:
    return status;

error:
    goto cleanup;
}


static
NTSTATUS
SamrSrvFillDomainInfo6(
    PDOMAIN_CONTEXT pDomCtx,
    PDIRECTORY_ENTRY pEntry,
    DomainInfo *pInfo
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    DomainInfo6 *pInfo6 = NULL;

    pInfo6 = &pInfo->info6;

    status = SamrSrvInitUnicodeString(&pInfo6->primary,
                                      NULL,
                                      pInfo);
    BAIL_ON_NTSTATUS_ERROR(status);

cleanup:
    return status;

error:
    goto cleanup;
}


static
NTSTATUS
SamrSrvFillDomainInfo7(
    PDOMAIN_CONTEXT pDomCtx,
    PDIRECTORY_ENTRY pEntry,
    DomainInfo *pInfo
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    DomainInfo7 *pInfo7 = NULL;

    pInfo7 = &pInfo->info7;
    pInfo7->role = SAMR_ROLE_DOMAIN_MEMBER;

    return status;
}


static
NTSTATUS
SamrSrvFillDomainInfo8(
    PDOMAIN_CONTEXT pDomCtx,
    PDIRECTORY_ENTRY pEntry,
    DomainInfo *pInfo
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    DWORD dwError = 0;
    WCHAR wszAttrCreatedTime[] = DS_ATTR_CREATED_TIME;
    LONG64 llCreatedTime = 0;
    DomainInfo8 *pInfo8 = NULL;

    dwError = DirectoryGetEntryAttrValueByName(pEntry,
                                               wszAttrCreatedTime,
                                               DIRECTORY_ATTR_TYPE_LARGE_INTEGER,
                                               &llCreatedTime);
    BAIL_ON_LSA_ERROR(dwError);

    pInfo8 = &pInfo->info8;
    pInfo8->domain_create_time = (NtTime)llCreatedTime;

cleanup:
    return status;

error:
    goto cleanup;
}


static
NTSTATUS
SamrSrvFillDomainInfo9(
    PDOMAIN_CONTEXT pDomCtx,
    PDIRECTORY_ENTRY pEntry,
    DomainInfo *pInfo
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    DomainInfo9 *pInfo9 = NULL;

    pInfo9 = &pInfo->info9;
    pInfo9->unknown = 0;

    return status;
}


static
NTSTATUS
SamrSrvFillDomainInfo11(
    PDOMAIN_CONTEXT pDomCtx,
    PDIRECTORY_ENTRY pEntry,
    DomainInfo *pInfo
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    DomainInfo11 *pInfo11 = NULL;

    pInfo11 = &pInfo->info11;

    status = SamrSrvFillDomainInfo2(pDomCtx,
                                   pEntry,
                                   (DomainInfo*)pInfo11);
    BAIL_ON_NTSTATUS_ERROR(status);

cleanup:
    return status;

error:
    goto cleanup;
}


static
NTSTATUS
SamrSrvFillDomainInfo12(
    PDOMAIN_CONTEXT pDomCtx,
    PDIRECTORY_ENTRY pEntry,
    DomainInfo *pInfo
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    return status;
}


static
NTSTATUS
SamrSrvFillDomainInfo13(
    PDOMAIN_CONTEXT pDomCtx,
    PDIRECTORY_ENTRY pEntry,
    DomainInfo *pInfo
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    DWORD dwError = 0;
    WCHAR wszAttrCreatedTime[] = DS_ATTR_CREATED_TIME;
    LONG64 llCreatedTime = 0;
    DomainInfo13 *pInfo13 = NULL;

    dwError = DirectoryGetEntryAttrValueByName(pEntry,
                                               wszAttrCreatedTime,
                                               DIRECTORY_ATTR_TYPE_LARGE_INTEGER,
                                               &llCreatedTime);
    BAIL_ON_LSA_ERROR(dwError);

    pInfo13 = &pInfo->info13;

    pInfo13->domain_create_time = (NtTime)llCreatedTime;

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
