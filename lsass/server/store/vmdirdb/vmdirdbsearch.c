/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 */

/*
 * Copyright Likewise Software
 * All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or (at
 * your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * for more details.  You should have received a copy of the GNU General
 * Public License along with this program.  If not, see
 * <http://www.gnu.org/licenses/>.
 *
 * LIKEWISE SOFTWARE MAKES THIS SOFTWARE AVAILABLE UNDER OTHER LICENSING
 * TERMS AS WELL.  IF YOU HAVE ENTERED INTO A SEPARATE LICENSE AGREEMENT
 * WITH LIKEWISE SOFTWARE, THEN YOU MAY ELECT TO USE THE SOFTWARE UNDER THE
 * TERMS OF THAT SOFTWARE LICENSE AGREEMENT INSTEAD OF THE TERMS OF THE GNU
 * GENERAL PUBLIC LICENSE, NOTWITHSTANDING THE ABOVE NOTICE.  IF YOU
 * HAVE QUESTIONS, OR WISH TO REQUEST A COPY OF THE ALTERNATE LICENSING
 * TERMS OFFERED BY LIKEWISE SOFTWARE, PLEASE CONTACT LIKEWISE SOFTWARE AT
 * license@likewisesoftware.com
 */

/*
 * Copyright (C) Likewise Software. All rights reserved.
 *
 * Module Name:
 *
 *        samdbsearch.c
 *
 * Abstract:
 *
 *
 *      Likewise VMDIR Database Provider
 *
 *      VMDIR objects searching routines
 *
 * Authors: Krishna Ganugapati (krishnag@likewise.com)
 *          Sriram Nambakam (snambakam@likewise.com)
 *          Rafal Szczesniak (rafal@likewise.com)
 *
 */

#include "includes.h"


DWORD
VmdirDbSearchObject(
    HANDLE            hDirectory,
    PWSTR             pwszBase,
    ULONG             ulScope,
    PWSTR             pwszFilter,
    PWSTR             wszAttributes[],
    ULONG             ulAttributesOnly,
    PDIRECTORY_ENTRY  *ppDirectoryEntries,
    PDWORD            pdwNumEntries
    )
{
    DWORD dwError = 0;
    NTSTATUS ntStatus = 0;
    DWORD dwAttrCount = 0;
    PVMDIR_AUTH_PROVIDER_CONTEXT pContext = NULL;
    LDAP *pLd = NULL;
    int ldap_err = 0;
    PSTR pszBase = NULL;
    PSTR pszBaseAlloc = NULL;
    PSTR pszFilter = NULL;
    PSTR *ppszAttributes = NULL;
/*    struct timeval timeout = {0}; */
    LDAPMessage *pRes = NULL;


    if (!hDirectory || !wszAttributes || !ppDirectoryEntries || !pdwNumEntries)
    {
        dwError = ERROR_INVALID_PARAMETER;
        BAIL_ON_VMDIRDB_ERROR(dwError);
    }

    pContext = (PVMDIR_AUTH_PROVIDER_CONTEXT) hDirectory;
    pLd = pContext->dirContext.pLd;

    if (pwszBase)
    {
        ntStatus = LwRtlCStringAllocateFromWC16String(&pszBaseAlloc, pwszBase);
        if (ntStatus)
        {
            dwError =  LwNtStatusToWin32Error(ntStatus);
            BAIL_ON_VMDIRDB_ERROR(dwError);
        }
        pszBase = pszBaseAlloc;
    }
    else
    {
        pszBase = pContext->dirContext.pBindInfo->pszSearchBase;
    }

    if (pwszFilter)
    {
        ntStatus = LwRtlCStringAllocateFromWC16String(&pszFilter, pwszFilter);
        if (ntStatus)
        {
            dwError =  LwNtStatusToWin32Error(ntStatus);
            BAIL_ON_VMDIRDB_ERROR(dwError);
        }
    }

    dwError = VmDirAttributesFromWc16Attributes(wszAttributes, 
                                                &ppszAttributes,
                                                &dwAttrCount);
    BAIL_ON_VMDIRDB_ERROR(dwError);

    ldap_err = ldap_search_ext_s(pLd,
                                 pszBase,
                                 ulScope,
                                 pszFilter,
                                 ppszAttributes,
                                 ulAttributesOnly,
                                 NULL, /* serverctrls */
                                 NULL, /* serverctrls */
                                 NULL, /* timeout */
                                 0,    /* sizelimit */
                                 &pRes);
    if (ldap_err)
    {
        dwError = LwMapLdapErrorToLwError(ldap_err);
        BAIL_ON_VMDIRDB_ERROR(dwError);
    }

cleanup:
    VmDirAttributesFree(&ppszAttributes);
    LW_SAFE_FREE_STRING(pszBaseAlloc);
    LW_SAFE_FREE_STRING(pszFilter);
    return dwError;

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
