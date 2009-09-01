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
 *        samr_deletealiasmember.c
 *
 * Abstract:
 *
 *        Remote Procedure Call (RPC) Server Interface
 *
 *        SamrDeleteAliasMember function
 *
 * Authors: Rafal Szczesniak (rafal@likewise.com)
 */

#include "includes.h"


NTSTATUS
SamrSrvDeleteAliasMember(
    handle_t IDL_handle,
    ACCOUNT_HANDLE hAlias,
    PSID pSid
    )
{
    const wchar_t wszFilterFmt[] = L"%ws='%ws'";

    NTSTATUS ntStatus = STATUS_SUCCESS;
    DWORD dwError = ERROR_SUCCESS;
    RPCSTATUS rpcStatus = 0;
    PACCOUNT_CONTEXT pAcctCtx = NULL;
    PDOMAIN_CONTEXT pDomCtx = NULL;
    PCONNECT_CONTEXT pConnCtx = NULL;
    PWSTR pwszGroupDn = NULL;
    PWSTR pwszSid = NULL;
    DWORD dwSidStrLen = 0;
    HANDLE hDirectory = NULL;
    PWSTR pwszBaseDn = NULL;
    WCHAR wszAttrDn[] = DS_ATTR_DISTINGUISHED_NAME;
    WCHAR wszAttrObjectClass[] = DS_ATTR_OBJECT_CLASS;
    WCHAR wszAttrObjectSid[] = DS_ATTR_OBJECT_SID;
    DWORD dwScope = 0;
    DWORD dwFilterLen = 0;
    PWSTR pwszFilter = NULL;
    PDIRECTORY_ENTRY pEntry = NULL;
    DWORD dwEntriesNum = 0;

    PWSTR wszAttributes[] = {
        wszAttrDn,
        wszAttrObjectClass,
        NULL
    };

    pAcctCtx = (PACCOUNT_CONTEXT)hAlias;

    if (pAcctCtx == NULL || pAcctCtx->Type != SamrContextAccount) {
        ntStatus = STATUS_INVALID_HANDLE;
        BAIL_ON_NTSTATUS_ERROR(ntStatus);
    }

    pDomCtx     = pAcctCtx->pDomCtx;
    pConnCtx    = pDomCtx->pConnCtx;
    hDirectory  = pConnCtx->hDirectory;
    pwszGroupDn = pAcctCtx->pwszDn;

    ntStatus = RtlAllocateWC16StringFromSid(&pwszSid,
                                            pSid);
    BAIL_ON_NTSTATUS_ERROR(ntStatus);

    dwError = LwWc16sLen(pwszSid, &dwSidStrLen);
    BAIL_ON_LSA_ERROR(dwError);

    dwFilterLen = ((sizeof(wszAttrObjectClass)/sizeof(WCHAR) - 1)) +
                  dwSidStrLen +
                  (sizeof(wszFilterFmt)/sizeof(wszFilterFmt[0]));

    dwError = LwAllocateMemory(sizeof(WCHAR) * dwFilterLen,
                               OUT_PPVOID(&pwszFilter));
    BAIL_ON_LSA_ERROR(dwError);

    sw16printfw(pwszFilter, dwFilterLen, wszFilterFmt,
                wszAttrObjectSid, pwszSid);

    dwError = DirectorySearch(hDirectory,
                              pwszBaseDn,
                              dwScope,
                              pwszFilter,
                              wszAttributes,
                              FALSE,
                              &pEntry,
                              &dwEntriesNum);
    BAIL_ON_LSA_ERROR(dwError);

    if (dwEntriesNum > 1)
    {
        ntStatus = STATUS_INTERNAL_ERROR;
    }
    else if (dwEntriesNum == 0)
    {
        ntStatus = STATUS_NO_SUCH_MEMBER;
    }
    BAIL_ON_NTSTATUS_ERROR(ntStatus);

    dwError = DirectoryRemoveFromGroup(hDirectory,
                                       pwszGroupDn,
                                       pEntry);

    if (dwError == LW_ERROR_MEMBER_NOT_IN_LOCAL_GROUP)
    {
        ntStatus = STATUS_MEMBER_NOT_IN_GROUP;
    }
    else if (dwError == ERROR_SUCCESS)
    {
        ntStatus = STATUS_SUCCESS;
    }
    else
    {
        ntStatus = STATUS_INTERNAL_ERROR;
    }
    BAIL_ON_NTSTATUS_ERROR(ntStatus);

cleanup:
    if (pEntry)
    {
        DirectoryFreeEntries(pEntry, dwEntriesNum);
    }

    LW_SAFE_FREE_MEMORY(pwszFilter);
    RTL_FREE(&pwszSid);

    return ntStatus;

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
