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
 *        samr_lookupdomain.c
 *
 * Abstract:
 *
 *        Remote Procedure Call (RPC) Server Interface
 *
 *        SamrLookupDomain function
 *
 * Authors: Rafal Szczesniak (rafal@likewise.com)
 */


#include "includes.h"


NTSTATUS SamrSrvLookupDomain(
    /* [in] */ handle_t hBinding,
    /* [in] */ CONNECT_HANDLE hConn,
    /* [in] */ UnicodeString *domain_name,
    /* [out] */ SID **ppSid
    )
{
    CHAR szDnToken[] = "DC";
    CHAR szFilter[] = "(objectclass=domain)";
    NTSTATUS status = STATUS_SUCCESS;
    DWORD dwError = 0;
    PCONNECT_CONTEXT pConn = NULL;
    PWSTR pwszBase = NULL;
    DWORD dwBaseLen = 0;
    DWORD dwScope = 0;
    PWSTR pwszFilter = NULL;
    PWSTR pwszAttrDomainSid = NULL;
    PWSTR wszAttributes[2];
    PDIRECTORY_ENTRY pEntries = NULL;
    DWORD dwCount = 0;
    PDIRECTORY_ATTRIBUTE pAttr = NULL;
    PATTRIBUTE_VALUE pAttrVal = NULL;
    PSID pDomainSid = NULL;

    memset(wszAttributes, 0, sizeof(wszAttributes));

    pConn = (PCONNECT_CONTEXT)hConn;

    if (pConn == NULL || pConn->Type != SamrContextConnect) {
        status = STATUS_INVALID_HANDLE;
        BAIL_ON_NTSTATUS_ERROR(status);
    }

    dwBaseLen = domain_name->size +
                ((sizeof(szDnToken) + 2) * sizeof(WCHAR));

    status = SamrSrvAllocateMemory((void**)&pwszBase,
                                   dwBaseLen,
                                   NULL);
    BAIL_ON_NTSTATUS_ERROR(status);

    sw16printf(pwszBase, "%s=%S",
               (PSTR)szDnToken,
               domain_name->string);

    dwError = LsaMbsToWc16s(szFilter, &pwszFilter);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaMbsToWc16s(DIRECTORY_ATTR_TAG_DOMAIN_SID,
                            &pwszAttrDomainSid);
    BAIL_ON_LSA_ERROR(dwError);

    wszAttributes[0] = pwszAttrDomainSid;

    dwError = DirectorySearch(pConn->hDirectory,
                              pwszBase,
                              dwScope,
                              pwszFilter,
                              wszAttributes,
                              TRUE,
                              &pEntries,
                              &dwCount);
    BAIL_ON_LSA_ERROR(dwError);

    if (dwCount == 1) {
        dwError = DirectoryGetEntryAttributeSingle(pEntries,
                                                   &pAttr);
        BAIL_ON_LSA_ERROR(dwError);

        dwError = DirectoryGetAttributeValue(pAttr, &pAttrVal);
        BAIL_ON_LSA_ERROR(dwError);

        if (pAttrVal->Type == DIRECTORY_ATTR_TYPE_UNICODE_STRING) {
            status = RtlAllocateSidFromWC16String(&pDomainSid,
                                                  pAttrVal->pwszStringValue);
            BAIL_ON_NTSTATUS_ERROR(status);

        } else {
            status = STATUS_INTERNAL_ERROR;
            BAIL_ON_NTSTATUS_ERROR(status);
        }
    }

    *ppSid = pDomainSid;

cleanup:
    if (pwszBase) {
        SamrSrvFreeMemory(pwszBase);
    }

    return status;

error:
    if (pDomainSid) {
        RTL_FREE(&pDomainSid);
    }

    *ppSid = NULL;
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
