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
 *        netlogondbopen.c
 *
 * Abstract:
 *
 *
 *      VMware netlogon LDAP Database Provider
 *
 *      Provider open functions
 *
 * Authors: Adam Bernstein (abernstein@vmware.com)
 *
 */

#include "includes.h"

DWORD
NetlogonLdapOpen(
    PHANDLE phDirectory
    )
{
    DWORD dwError = 0;
    PNETLOGON_AUTH_PROVIDER_CONTEXT pContext = NULL;
    PNETLOGON_LDAP_BIND_INFO pBindInfo = NULL;
    LDAP             *pLd = NULL;

    if (!phDirectory)
    {
        dwError = ERROR_INVALID_PARAMETER;
        BAIL_ON_NETLOGON_LDAP_ERROR(dwError);
    }

    dwError = LwAllocateMemory(
                    sizeof(NETLOGON_AUTH_PROVIDER_CONTEXT),
                    (PVOID*)&pContext);
    BAIL_ON_NETLOGON_LDAP_ERROR(dwError);
 
    pthread_mutex_init(&pContext->mutex, NULL);
    pContext->pMutex = &pContext->mutex;

    /* 
     * Call NetlogonCreateBindInfo()
     * Call NetlogonLdapInitialize()
     * Wrap up this information in the phDirectory and return to caller.
     */
    dwError = NetlogonCreateBindInfo(&pBindInfo);
    BAIL_ON_NETLOGON_LDAP_ERROR(dwError);

    dwError = NetlogonLdapInitialize(
                  pBindInfo->pszURI,
                  pBindInfo->pszUPN,
                  pBindInfo->pszPassword,
                  VMDIR_KRB5_CC_NAME,
                  &pLd);
    BAIL_ON_NETLOGON_LDAP_ERROR(dwError);

    pContext->dirContext.pBindInfo = pBindInfo;
    pBindInfo = NULL;
    
    pContext->dirContext.pLd = pLd;
    pLd = NULL;

    *phDirectory = pContext;

cleanup:

    return dwError;

error:
    NetlogonReleaseBindInfo(pBindInfo);
    if (pLd)
    {
        NetlogonLdapClose(pLd);
    }

    goto cleanup;
}

VOID
NetlogonLdapRelease(
    PHANDLE phDirectory)
{
    PNETLOGON_AUTH_PROVIDER_CONTEXT pContext = NULL;
    pContext = (PNETLOGON_AUTH_PROVIDER_CONTEXT) phDirectory;

    if (!phDirectory)
    {
        return;
    }

    pthread_mutex_destroy(pContext->pMutex);
    NetlogonReleaseBindInfo(pContext->dirContext.pBindInfo);
    if (pContext->dirContext.pLd)
    {
        ldap_unbind_ext(pContext->dirContext.pLd, NULL, NULL);
    }
    LW_SAFE_FREE_MEMORY(pContext);
}


/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
