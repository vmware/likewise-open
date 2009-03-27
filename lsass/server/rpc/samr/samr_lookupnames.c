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
 *        samr_lookupnames.c
 *
 * Abstract:
 *
 *        Remote Procedure Call (RPC) Server Interface
 *
 *        SamrLookupNames function
 *
 * Authors: Rafal Szczesniak (rafal@likewise.com)
 */

#include "includes.h"


NTSTATUS
SamrSrvLookupNames(
    /* [in] */ handle_t hBinding,
    /* [in] */ DOMAIN_HANDLE hDomain,
    /* [in] */ uint32 num_names,
    /* [in] */ UnicodeString *names,
    /* [out] */ Ids *ids,
    /* [out] */ Ids *types
    )
{
    CHAR szFilterFmt[] = "(|(&(objectclass=user)(user-name=%S))"
                           "(&(objectclass=group)(group-name=%S)))";

    NTSTATUS status = STATUS_SUCCESS;
    DWORD dwError = 0;
    PDOMAIN_CONTEXT pDomCtx = NULL;
    HANDLE hDirectory = NULL;
    Ids *pIds = NULL;
    Ids *pTypes = NULL;
    PWSTR pwszDn = NULL;
    DWORD dwScope = 0;
    PWSTR pwszFilter = NULL;
    PWSTR wszAttributes[3];
    PWSTR pwszAttrNameGid = NULL;
    PWSTR pwszAttrNameUid = NULL;
    DWORD i = 0;
    PWSTR pwszName = NULL;
    PDIRECTORY_ENTRY pEntries = NULL;
    PDIRECTORY_ENTRY pEntry = NULL;
    PDIRECTORY_ATTRIBUTE pAttr = NULL;
    PATTRIBUTE_VALUE pAttrVal = NULL;
    DWORD dwEntriesNum = 0;

    pDomCtx = (PDOMAIN_CONTEXT)hDomain;

    if (pDomCtx == NULL || pDomCtx->Type != SamrContextDomain) {
        status = STATUS_INVALID_HANDLE;
        BAIL_ON_NTSTATUS_ERROR(status);
    }

    hDirectory = pDomCtx->pConnCtx->hDirectory;
    pwszDn     = pDomCtx->pwszDn;

    status = SamrSrvAllocateMemory((void**)&pIds,
                                   sizeof(*pIds),
                                   pDomCtx);
    BAIL_ON_NTSTATUS_ERROR(status);

    pIds->count = num_names;
    status = SamrSrvAllocateMemory((void**)&(pIds->ids),
                                   pIds->count * sizeof(uint32),
                                   pDomCtx);
    BAIL_ON_NTSTATUS_ERROR(status);

    status = SamrSrvAllocateMemory((void**)&pTypes,
                                   sizeof(*pTypes),
                                   pDomCtx);
    BAIL_ON_NTSTATUS_ERROR(status);

    pTypes->count = num_names;
    status = SamrSrvAllocateMemory((void**)&(pTypes->ids),
                                   pTypes->count * sizeof(uint32),
                                   pDomCtx);
    BAIL_ON_NTSTATUS_ERROR(status);

    dwError = LsaMbsToWc16s(DIRECTORY_ATTR_TAG_GID, &pwszAttrNameGid);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaMbsToWc16s(DIRECTORY_ATTR_TAG_UID, &pwszAttrNameUid);
    BAIL_ON_LSA_ERROR(dwError);

    for (i = 0; i < num_names; i++) {
        UnicodeString *name = &(names[i]);

        status = SamrSrvAllocateMemory((void**)&pwszName,
                                       name->size * sizeof(WCHAR),
                                       NULL);
        BAIL_ON_NTSTATUS_ERROR(status);

        wc16sncpy(pwszName, name->string, name->len);

        status = SamrSrvAllocateMemory((void**)&pwszFilter,
                                       (name->size * 2 + sizeof(szFilterFmt))
                                       * sizeof(WCHAR),
                                       NULL);
        BAIL_ON_NTSTATUS_ERROR(status);

        sw16printf(pwszFilter, szFilterFmt, pwszName);

        wszAttributes[0] = pwszAttrNameUid;
        wszAttributes[1] = pwszAttrNameGid;
        wszAttributes[2] = NULL;

        pEntry   = NULL;
        pAttr    = NULL;
        pAttrVal = NULL;

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

            dwError = DirectoryGetEntryAttributeByName(pEntry,
                                                       pwszAttrNameUid,
                                                       &pAttr);
            if (pAttr && dwError == 0) {
                dwError = DirectoryGetAttributeValue(pAttr,
                                                     &pAttrVal);
                BAIL_ON_LSA_ERROR(dwError);

                if (pAttrVal &&
                    pAttrVal->Type == DIRECTORY_ATTR_TYPE_LARGE_INTEGER) {

                    pIds->ids[i]   = pAttrVal->ulValue;
                    pTypes->ids[i] = SID_TYPE_USER;

                } else {
                    status = STATUS_INTERNAL_ERROR;
                    BAIL_ON_NTSTATUS_ERROR(status);
                }

            } else {
                pAttr = NULL;
                dwError = DirectoryGetEntryAttributeByName(pEntry,
                                                           pwszAttrNameGid,
                                                           &pAttr);
                BAIL_ON_LSA_ERROR(dwError);

                dwError = DirectoryGetAttributeValue(pAttr,
                                                     &pAttrVal);
                BAIL_ON_LSA_ERROR(dwError);

                if (pAttrVal &&
                    pAttrVal->Type == DIRECTORY_ATTR_TYPE_LARGE_INTEGER) {

                    pIds->ids[i]   = pAttrVal->ulValue;
                    pTypes->ids[i] = SID_TYPE_ALIAS;

                } else {
                    status = STATUS_INTERNAL_ERROR;
                    BAIL_ON_NTSTATUS_ERROR(status);
                }
            }

        } else if (dwEntriesNum == 0) {
            pIds->ids[i]   = 0;
            pTypes->ids[i] = SID_TYPE_UNKNOWN;

        } else {
            status = STATUS_INTERNAL_ERROR;
            BAIL_ON_NTSTATUS_ERROR(status);
        }

        if (pEntries) {
            DirectoryFreeEntries(pEntries, dwEntriesNum);
            pEntries = NULL;
        }

        if (pwszFilter) {
            SamrSrvFreeMemory(pwszFilter);
            pwszFilter = NULL;
        }

        if (pwszName) {
            SamrSrvFreeMemory(pwszName);
            pwszName = NULL;
        }
    }

    ids->count   = pIds->count;
    ids->ids     = pIds->ids;
    types->count = pTypes->count;
    types->ids   = pTypes->ids;

cleanup:
    if (pwszAttrNameGid) {
        LSA_SAFE_FREE_MEMORY(pwszAttrNameGid);
    }

    if (pwszAttrNameGid) {
        LSA_SAFE_FREE_MEMORY(pwszAttrNameUid);
    }

    if (pwszName) {
        SamrSrvFreeMemory(pwszName);
    }

    if (pwszFilter) {
        SamrSrvFreeMemory(pwszFilter);
    }

    if (pEntries) {
        DirectoryFreeEntries(pEntries, dwEntriesNum);
    }

    if (pIds) {
        SamrSrvFreeMemory(pIds);
    }

    if (pTypes) {
        SamrSrvFreeMemory(pTypes);
    }

    return status;

error:
    if (pIds->ids) {
        SamrSrvFreeMemory(pIds->ids);
    }

    if (pTypes->ids) {
        SamrSrvFreeMemory(pTypes->ids);
    }

    ids->count   = 0;
    ids->ids     = NULL;
    types->count = 0;
    types->ids   = NULL;

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
