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
 *        samr_opendomain.c
 *
 * Abstract:
 *
 *        Remote Procedure Call (RPC) Server Interface
 *
 *        SamrOpenDomain function
 *
 * Authors: Rafal Szczesniak (rafal@likewise.com)
 */

#include "includes.h"


NTSTATUS
SamrSrvOpenDomain(
    /* [in] */ handle_t hBinding,
    /* [in] */ CONNECT_HANDLE hConn,
    /* [in] */ uint32 access_mask,
    /* [in] */ SID *sid,
    /* [out] */ DOMAIN_HANDLE *hDomain
    )
{
    CHAR szFilter[] = "(objectclass=domain)";
    NTSTATUS status = STATUS_SUCCESS;
    DWORD dwError = 0;
    PCONNECT_CONTEXT pConnCtx = NULL;
    PDOMAIN_CONTEXT pDomCtx = NULL;
    PWSTR pwszBase = NULL;
    DWORD dwScope = 0;
    PWSTR pwszFilter = NULL;
    PWSTR wszAttributes[3];
    PWSTR pwszAttrDomainName = NULL;
    PWSTR pwszAttrDomainSid = NULL;
    PDIRECTORY_ENTRY pEntries = NULL;
    DWORD dwEntriesNum = 0;
    PDIRECTORY_ENTRY pEntry = NULL;
    PDIRECTORY_ATTRIBUTE pAttrDomainName = NULL;
    PDIRECTORY_ATTRIBUTE pAttrDomainSid = NULL;
    PDIRECTORY_ATTRIBUTE pAttrDn = NULL;
    PATTRIBUTE_VALUE pAttrVal = NULL;
    DWORD i = 0;
    PSID pDomainSid = NULL;
    PWSTR pwszDomainName = NULL;
    DWORD dwNameLen = 0;
    PWSTR pwszDn = NULL;
    DWORD dwDnLen = 0;

    pConnCtx = (PCONNECT_CONTEXT)hConn;

    if (pConnCtx == NULL || pConnCtx->Type != SamrContextConnect) {
        status = STATUS_INVALID_HANDLE;
        BAIL_ON_NTSTATUS_ERROR(status);
    }

    dwError = LsaMbsToWc16s(szFilter, &pwszFilter);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaMbsToWc16s(DIRECTORY_ATTR_TAG_DOMAIN_NAME,
                            &pwszAttrDomainName);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaMbsToWc16s(DIRECTORY_ATTR_TAG_DOMAIN_SID,
                            &pwszAttrDomainSid);
    BAIL_ON_LSA_ERROR(dwError);

    wszAttributes[0] = pwszAttrDomainName;
    wszAttributes[1] = pwszAttrDomainSid;
    wszAttributes[2] = NULL;

    dwError = DirectorySearch(pConnCtx->hDirectory,
                              pwszBase,
                              dwScope,
                              pwszFilter,
                              wszAttributes,
                              TRUE,
                              &pEntries,
                              &dwEntriesNum);
    BAIL_ON_LSA_ERROR(dwError);

    status = SamrSrvAllocateMemory((void**)&pDomCtx,
                                   sizeof(*pDomCtx),
                                   NULL);
    BAIL_ON_NTSTATUS_ERROR(status);

    for (i = 0; i < dwEntriesNum; i++) {
        pEntry = &(pEntries[i]);
        dwError = DirectoryGetEntryAttributeByName(pEntry,
                                                   pwszAttrDomainSid,
                                                   &pAttrDomainSid);
        BAIL_ON_LSA_ERROR(dwError);

        dwError = DirectoryGetAttributeValue(pAttrDomainSid,
                                             &pAttrVal);
        BAIL_ON_LSA_ERROR(dwError);

        if (pAttrVal &&
            pAttrVal->Type == DIRECTORY_ATTR_TYPE_UNICODE_STRING) {

            status = RtlAllocateSidFromWC16String(&pDomainSid,
                                                  pAttrVal->pwszStringValue);
            BAIL_ON_NTSTATUS_ERROR(status);

            status = SamrSrvAddDepMemory(pDomainSid, pDomCtx);
            BAIL_ON_NTSTATUS_ERROR(status);

            pAttrVal = NULL;

            if (RtlEqualSid(pDomainSid, sid)) {
                dwError = DirectoryGetEntryAttributeByName(pEntry,
                                                           pwszAttrDomainName,
                                                           &pAttrDomainName);
                BAIL_ON_LSA_ERROR(dwError);

                dwError = DirectoryGetAttributeValue(pAttrDomainName,
                                                     &pAttrVal);
                BAIL_ON_LSA_ERROR(dwError);

            } else {
                status = STATUS_INVALID_SID;
                BAIL_ON_NTSTATUS_ERROR(status);
            }

            if (pAttrVal &&
                pAttrVal->Type == DIRECTORY_ATTR_TYPE_UNICODE_STRING) {

                dwNameLen = wc16slen(pAttrVal->pwszStringValue);
                status = SamrSrvAllocateMemory((void**)&pwszDomainName,
                                               (dwNameLen + 1) * sizeof(wchar16_t),
                                               pDomCtx);
                BAIL_ON_NTSTATUS_ERROR(status);

                wc16sncpy(pwszDomainName, pAttrVal->pwszStringValue, dwNameLen);

            } else  {
                status = STATUS_INTERNAL_ERROR;
                BAIL_ON_NTSTATUS_ERROR(status);
            }


            dwError = DirectoryGetEntryAttributeByNameA(pEntry,
                                                        DIRECTORY_ATTR_TAG_DN,
                                                        &pAttrDn);
            BAIL_ON_LSA_ERROR(dwError);

            pAttrVal = NULL;
            dwError = DirectoryGetAttributeValue(pAttrDn,
                                                 &pAttrVal);
            if (pAttrVal &&
                pAttrVal->Type == DIRECTORY_ATTR_TYPE_UNICODE_STRING) {

                dwDnLen = wc16slen(pAttrVal->pwszStringValue);
                status = SamrSrvAllocateMemory((void**)&pwszDn,
                                               (dwDnLen + 1) * sizeof(wchar16_t),
                                               pDomCtx);
                BAIL_ON_NTSTATUS_ERROR(status);

                wc16sncpy(pwszDn, pAttrVal->pwszStringValue, dwDnLen);

            } else {
                status = STATUS_INTERNAL_ERROR;
                BAIL_ON_NTSTATUS_ERROR(status);
            }

        } else {
            status = STATUS_INTERNAL_ERROR;
            BAIL_ON_NTSTATUS_ERROR(status);
        }
    }

    pDomCtx->Type           = SamrContextDomain;
    pDomCtx->refcount       = 1;
    pDomCtx->pDomainSid     = pDomainSid;
    pDomCtx->pwszDomainName = pwszDomainName;
    pDomCtx->pwszDn         = pwszDn;
    pDomCtx->pConnCtx       = pConnCtx;

    InterlockedIncrement(&pConnCtx->refcount);

    *hDomain = (DOMAIN_HANDLE)pDomCtx;

cleanup:
    if (pwszAttrDomainName) {
        LSA_SAFE_FREE_MEMORY(pwszAttrDomainName);
    }

    if (pwszAttrDomainSid) {
        LSA_SAFE_FREE_MEMORY(pwszAttrDomainSid);
    }

    if (pwszFilter) {
        LSA_SAFE_FREE_MEMORY(pwszFilter);
    }

    if (pEntries) {
        DirectoryFreeEntries(pEntries, dwEntriesNum);
    }

    return status;

error:
    if (pDomCtx) {
        InterlockedDecrement(&pConnCtx->refcount);
        DOMAIN_HANDLE_rundown((DOMAIN_HANDLE)pDomCtx);
    }

    hDomain = NULL;
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
