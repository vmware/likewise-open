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
    wchar_t wszFilter[] = L"(%ws=%d OR %ws=%d) AND %ws='%ws'";
    NTSTATUS status = STATUS_SUCCESS;
    DWORD dwError = 0;
    PCONNECT_CONTEXT pConnCtx = NULL;
    PDOMAIN_CONTEXT pDomCtx = NULL;
    PWSTR pwszBase = NULL;
    WCHAR wszAttrObjectClass[] = DS_ATTR_OBJECT_CLASS;
    WCHAR wszAttrObjectSid[] = DS_ATTR_OBJECT_SID;
    WCHAR wszAttrCommonName[] = DS_ATTR_COMMON_NAME;
    WCHAR wszAttrDn[] = DS_ATTR_DISTINGUISHED_NAME;
    DWORD dwObjectClassDomain = DS_OBJECT_CLASS_DOMAIN;
    DWORD dwObjectClassBuiltin = DS_OBJECT_CLASS_BUILTIN_DOMAIN;
    DWORD dwScope = 0;
    PWSTR pwszFilter = NULL;
    DWORD dwFilterLen = 0;
    PWSTR wszAttributes[4];
    PWSTR pwszDomainSid = NULL;
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

    BAIL_ON_INVALID_PARAMETER(sid);
    BAIL_ON_INVALID_PARAMETER(hDomain);

    pConnCtx = (PCONNECT_CONTEXT)hConn;

    if (pConnCtx == NULL || pConnCtx->Type != SamrContextConnect) {
        status = STATUS_INVALID_HANDLE;
        BAIL_ON_NTSTATUS_ERROR(status);
    }

    status = RtlAllocateWC16StringFromSid(&pwszDomainSid, sid);
    BAIL_ON_NTSTATUS_ERROR(status);

    dwFilterLen = ((sizeof(wszAttrObjectClass) / sizeof(WCHAR)) - 1) +
                  10 +
                  (sizeof(wszFilter) / sizeof(wszFilter[0])) +
                  ((sizeof(wszAttrObjectClass) / sizeof(WCHAR)) - 1) +
                  10 +
                  ((sizeof(wszAttrObjectSid) / sizeof(WCHAR)) - 1) +
                  wc16slen(pwszDomainSid);

    status = SamrSrvAllocateMemory((void**)&pwszFilter,
                                   dwFilterLen * sizeof(*pwszFilter));
    BAIL_ON_NTSTATUS_ERROR(status);

    sw16printfw(pwszFilter, dwFilterLen, wszFilter,
                &wszAttrObjectClass[0],
                dwObjectClassDomain,
                &wszAttrObjectClass[0],
                dwObjectClassBuiltin,
                &wszAttrObjectSid[0],
                pwszDomainSid);

    wszAttributes[0] = wszAttrCommonName;
    wszAttributes[1] = wszAttrObjectSid;
    wszAttributes[2] = wszAttrDn;
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

    RTL_ALLOCATE(&pDomCtx, DOMAIN_CONTEXT, sizeof(*pDomCtx));
    BAIL_ON_NO_MEMORY(pDomCtx);

    for (i = 0; i < dwEntriesNum; i++) {
        pEntry = &(pEntries[i]);
        dwError = DirectoryGetEntryAttributeByName(pEntry,
                                                   wszAttrObjectSid,
                                                   &pAttrDomainSid);
        BAIL_ON_LSA_ERROR(dwError);

        dwError = DirectoryGetAttributeValue(pAttrDomainSid,
                                             &pAttrVal);
        BAIL_ON_LSA_ERROR(dwError);

        if (pAttrVal &&
            pAttrVal->Type == DIRECTORY_ATTR_TYPE_UNICODE_STRING) {

            status = RtlAllocateSidFromWC16String(
                                     &pDomainSid,
                                     pAttrVal->data.pwszStringValue);
            BAIL_ON_NTSTATUS_ERROR(status);

            pAttrVal = NULL;

            if (RtlEqualSid(pDomainSid, sid)) {
                dwError = DirectoryGetEntryAttributeByName(pEntry,
                                                           wszAttrCommonName,
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

                dwNameLen = wc16slen(pAttrVal->data.pwszStringValue);
                RTL_ALLOCATE(&pwszDomainName,
                             WCHAR,
                             (dwNameLen + 1) * sizeof(WCHAR));
                BAIL_ON_NO_MEMORY(pwszDomainName);

                wc16sncpy(pwszDomainName, pAttrVal->data.pwszStringValue, dwNameLen);

            } else  {
                status = STATUS_INTERNAL_ERROR;
                BAIL_ON_NTSTATUS_ERROR(status);
            }


            dwError = DirectoryGetEntryAttributeByName(pEntry,
                                                       wszAttrDn,
                                                       &pAttrDn);
            BAIL_ON_LSA_ERROR(dwError);

            pAttrVal = NULL;
            dwError = DirectoryGetAttributeValue(pAttrDn,
                                                 &pAttrVal);
            if (pAttrVal &&
                pAttrVal->Type == DIRECTORY_ATTR_TYPE_UNICODE_STRING) {

                dwDnLen = wc16slen(pAttrVal->data.pwszStringValue);
                RTL_ALLOCATE(&pwszDn,
                             WCHAR,
                             (dwDnLen + 1) * sizeof(WCHAR));
                BAIL_ON_NO_MEMORY(pwszDomainName);

                wc16sncpy(pwszDn, pAttrVal->data.pwszStringValue, dwDnLen);

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
    if (pwszDomainSid) {
        RTL_FREE(&pwszDomainSid);
    }

    if (pwszFilter) {
        SamrSrvFreeMemory(pwszFilter);
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
