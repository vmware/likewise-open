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
 *        samr_openuser.c
 *
 * Abstract:
 *
 *        Remote Procedure Call (RPC) Server Interface
 *
 *        SamrOpenUser function
 *
 * Authors: Rafal Szczesniak (rafal@likewise.com)
 */

#include "includes.h"


NTSTATUS
SamrSrvOpenUser(
    /* [in] */ handle_t hBinding,
    /* [in] */ DOMAIN_HANDLE *hDomain,
    /* [in] */ uint32 access_mask,
    /* [in] */ uint32 rid,
    /* [out] */ ACCOUNT_HANDLE *hUser
    )
{
    CHAR szFilterFmt[] = "(&(objectclass=user)(uid=%d))";
    NTSTATUS status = STATUS_SUCCESS;
    DWORD dwError = 0;
    PDOMAIN_CONTEXT pDomCtx = NULL;
    PACCOUNT_CONTEXT pAccCtx = NULL;
    HANDLE hDirectory = NULL;
    PWSTR pwszAttrNameUsername = NULL;
    PWSTR pwszAttrNameSid = NULL;
    PWSTR pwszDn = NULL;
    DWORD dwScope = 0;
    PWSTR pwszFilter = NULL;
    PWSTR wszAttributes[3];
    PDIRECTORY_ENTRY pEntries = NULL;
    PDIRECTORY_ENTRY pEntry = NULL;
    PDIRECTORY_ATTRIBUTE pAttr = NULL;
    PATTRIBUTE_VALUE pAttrVal = NULL;
    DWORD dwEntriesNum = 0;
    DWORD i = 0;
    PWSTR pwszName = NULL;
    DWORD dwNameLen = 0;
    PSID pSid = NULL;

    pDomCtx = (PDOMAIN_CONTEXT)hDomain;

    if (pDomCtx == NULL || pDomCtx->Type != SamrContextDomain) {
        status = STATUS_INVALID_HANDLE;
        BAIL_ON_NTSTATUS_ERROR(status);
    }

    hDirectory = pDomCtx->pConnCtx->hDirectory;
    pwszDn     = pDomCtx->pwszDn;

    dwError = LsaMbsToWc16s(DIRECTORY_ATTR_TAG_USER_NAME,
                            &pwszAttrNameUsername);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaMbsToWc16s(DIRECTORY_ATTR_TAG_USER_SID,
                            &pwszAttrNameSid);
    BAIL_ON_LSA_ERROR(dwError);

    status = SamrSrvAllocateMemory((void**)&pAccCtx,
                                   sizeof(*pAccCtx),
                                   NULL);
    BAIL_ON_NTSTATUS_ERROR(status);

    status = SamrSrvAllocateMemory((void**)&pwszFilter,
                                   (sizeof (szFilterFmt) + 10)
                                   * sizeof(WCHAR),
                                   NULL);
    BAIL_ON_NTSTATUS_ERROR(status);

    sw16printf(pwszFilter, szFilterFmt, rid);

    wszAttributes[0] = pwszAttrNameUsername;
    wszAttributes[1] = pwszAttrNameSid;
    wszAttributes[2] = NULL;

    dwError = DirectorySearch(hDirectory,
                              pwszDn,
                              dwScope,
                              pwszFilter,
                              wszAttributes,
                              TRUE,
                              &pEntries,
                              &dwEntriesNum);
    BAIL_ON_LSA_ERROR(dwError);

    if (dwEntriesNum == 1) {
        pEntry = &(pEntries[0]);

        for (i = 0; pEntry->ulNumAttributes; i++) {
            pAttr = &(pEntry->pAttributes[i]);

            dwError = DirectoryGetAttributeValue(pAttr,
                                                 &pAttrVal);
            BAIL_ON_LSA_ERROR(dwError);

            if (!wc16scmp(pAttr->pwszName, pwszAttrNameUsername) &&
                pAttrVal->Type == DIRECTORY_ATTR_TYPE_UNICODE_STRING) {

                dwNameLen = wc16slen(pAttrVal->pwszStringValue);

                status = SamrSrvAllocateMemory((void**)&pwszName,
                                               (dwNameLen + 1) * sizeof(WCHAR),
                                               pAccCtx);
                BAIL_ON_NTSTATUS_ERROR(status);

                wc16sncpy(pwszName, pAttrVal->pwszStringValue, dwNameLen);


            } else if (!wc16scmp(pAttr->pwszName, pwszAttrNameSid) &&
                pAttrVal->Type == DIRECTORY_ATTR_TYPE_UNICODE_STRING) {

                status = RtlAllocateSidFromWC16String(&pSid,
                                                      pAttrVal->pwszStringValue);
                BAIL_ON_NTSTATUS_ERROR(status);
            }
        }

    } else if (dwEntriesNum == 0) {
        status = STATUS_NO_SUCH_USER;
        BAIL_ON_NTSTATUS_ERROR(status);

    } else {
        status = STATUS_INTERNAL_ERROR;
        BAIL_ON_NTSTATUS_ERROR(status);
    }

    pAccCtx->Type          = SamrContextAccount;
    pAccCtx->refcount      = 1;
    pAccCtx->pwszName      = pwszName;
    pAccCtx->pSid          = pSid;
    pAccCtx->dwAccountType = SID_TYPE_USER;

    InterlockedIncrement(&pAccCtx->refcount);

    *hUser = (ACCOUNT_HANDLE)pAccCtx;

cleanup:
    if (pwszAttrNameUsername) {
        SamrSrvFreeMemory(pwszAttrNameUsername);
    }

    if (pwszAttrNameSid) {
        SamrSrvFreeMemory(pwszAttrNameSid);
    }

    if (pwszFilter) {
        SamrSrvFreeMemory(pwszFilter);
    }

    if (pEntries) {
        DirectoryFreeEntries(pEntries, dwEntriesNum);
    }

    return status;

error:
    if (pAccCtx) {
        InterlockedDecrement(&pAccCtx->refcount);
        ACCOUNT_HANDLE_rundown((ACCOUNT_HANDLE)pAccCtx);
    }

    if (pSid) {
        RTL_FREE(&pSid);
    }

    *hUser = NULL;
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
