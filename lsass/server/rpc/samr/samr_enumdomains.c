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
 *        samr_enumdomains.c
 *
 * Abstract:
 *
 *        Remote Procedure Call (RPC) Server Interface
 *
 *        SamrEnumDomains function
 *
 * Authors: Rafal Szczesniak (rafal@likewise.com)
 */

#include "includes.h"


NTSTATUS
SamrSrvEnumDomains(
    /* [in] */ handle_t hBinding,
    /* [in] */ CONNECT_HANDLE hConn,
    /* [in, out] */ uint32 *resume,
    /* [in] */ uint32 size,
    /* [out] */ EntryArray **domains,
    /* [out] */ uint32 *num_entries
    )
{
    wchar_t wszFilter[] = L"%ws=%d OR %ws=%d";
    NTSTATUS status = STATUS_SUCCESS;
    DWORD dwError = 0;
    PCONNECT_CONTEXT pConnCtx = NULL;
    PWSTR pwszBase = NULL;
    WCHAR wszAttrObjectClass[] = DS_ATTR_OBJECT_CLASS;
    WCHAR wszAttrCommonName[] = DS_ATTR_COMMON_NAME;
    DWORD dwObjectClassDomain = DS_OBJECT_CLASS_DOMAIN;
    DWORD dwObjectClassBuiltin = DS_OBJECT_CLASS_BUILTIN_DOMAIN;
    DWORD dwScope = 0;
    PWSTR pwszFilter = NULL;
    DWORD dwFilterLen = 0;
    PWSTR wszAttributes[2];
    PDIRECTORY_ENTRY pEntries = NULL;
    DWORD dwEntriesNum = 0;
    PDIRECTORY_ENTRY pEntry = NULL;
    PDIRECTORY_ATTRIBUTE pAttr = NULL;
    PATTRIBUTE_VALUE pAttrVal = NULL;
    DWORD i = 0;
    DWORD dwCount = 0;
    DWORD dwSize = 0;
    EntryArray *pDomains = NULL;

    pConnCtx = (PCONNECT_CONTEXT)hConn;

    if (pConnCtx == NULL || pConnCtx->Type != SamrContextConnect) {
        status = STATUS_INVALID_HANDLE;
        BAIL_ON_NTSTATUS_ERROR(status);
    }

    dwFilterLen = ((sizeof(wszAttrObjectClass) / sizeof(WCHAR)) - 1) +
                  10 +
                  (sizeof(wszFilter) / sizeof(wszFilter[0])) +
                  ((sizeof(wszAttrObjectClass) / sizeof(WCHAR)) - 1) +
                  10;

    status = SamrSrvAllocateMemory((void**)&pwszFilter,
                                   dwFilterLen * sizeof(*pwszFilter),
                                   pConnCtx);
    BAIL_ON_NTSTATUS_ERROR(status);

    sw16printfw(pwszFilter, dwFilterLen, wszFilter,
                &wszAttrObjectClass[0],
                dwObjectClassDomain,
                &wszAttrObjectClass[0],
                dwObjectClassBuiltin);

    wszAttributes[0] = wszAttrCommonName;
    wszAttributes[1] = NULL;

    dwError = DirectorySearch(pConnCtx->hDirectory,
                              pwszBase,
                              dwScope,
                              pwszFilter,
                              wszAttributes,
                              FALSE,
                              &pEntries,
                              &dwEntriesNum);
    BAIL_ON_LSA_ERROR(dwError);

    status = SamrSrvAllocateMemory((void**)&pDomains,
                                   sizeof(EntryArray),
                                   pConnCtx);
    BAIL_ON_NTSTATUS_ERROR(status);

    //    dwSize += sizeof(pDomains->count);

    i       = *resume;
    dwCount = 0;

    if (i >= dwEntriesNum) {
        i = 0;
        status = STATUS_NO_MORE_ENTRIES;
    }

    while (dwSize < size && i < dwEntriesNum) {
        pEntry = &(pEntries[i]);

        dwError = DirectoryGetEntryAttributeSingle(pEntry,
                                                   &pAttr);
        BAIL_ON_LSA_ERROR(dwError);

        dwError = DirectoryGetAttributeValue(pAttr, &pAttrVal);
        BAIL_ON_LSA_ERROR(dwError);

        if (pAttrVal->Type == DIRECTORY_ATTR_TYPE_UNICODE_STRING) {
            //            dwSize += sizeof(uint32);
            dwSize += wc16slen(pAttrVal->data.pwszStringValue) * sizeof(wchar16_t);
            //            dwSize += 2 * sizeof(uint16);

            if (dwSize < size && pDomains->entries) {
                status = SamrSrvReallocMemory((void**)&pDomains->entries,
                                              sizeof(pDomains->entries[0]) * (++dwCount),
                                              pDomains->entries);

            } else if (dwSize < size && !pDomains->entries) {
                status = SamrSrvAllocateMemory((void**)&pDomains->entries,
                                               sizeof(pDomains->entries[0]) * (++dwCount),
                                               pDomains);
            } else {
                status = STATUS_MORE_ENTRIES;
                break;
            }

            BAIL_ON_NTSTATUS_ERROR(status);

            pDomains->count = dwCount;
            pDomains->entries[dwCount - 1].idx = i;

            status = SamrSrvInitUnicodeString(&(pDomains->entries[dwCount - 1].name),
                                              pAttrVal->data.pwszStringValue,
                                              pDomains->entries);
            BAIL_ON_NTSTATUS_ERROR(status);

        } else {
            status = STATUS_INTERNAL_ERROR;
            BAIL_ON_NTSTATUS_ERROR(status);
        }

        i++;
    }

    *resume      = i;
    *num_entries = dwCount;
    *domains     = pDomains;

cleanup:
    if (pwszFilter) {
        SamrSrvFreeMemory(pwszFilter);
    }

    if (pEntries) {
        DirectoryFreeEntries(pEntries, dwEntriesNum);
    }

    return status;

error:
    if (pDomains) {
        SamrSrvFreeMemory(pDomains);
    }

    *resume      = 0;
    *num_entries = 0;
    *domains     = NULL;
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
