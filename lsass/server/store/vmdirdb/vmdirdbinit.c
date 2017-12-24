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
 *        vmdirdbinit.c
 *
 * Abstract:
 *
 *
 *      VMware vmdir LDAP Database Provider
 *
 *      Database initialisation routines
 *
 * Authors: Adam Bernstein (abernstein@vmware.com)
 *
 */

#include "includes.h"



DWORD
DirectoryInitializeProvider(
    PSTR* ppszProviderName,
    PDIRECTORY_PROVIDER_FUNCTION_TABLE* ppFnTable
    )
{
    DWORD dwError = 0;
    DIRECTORY_PROVIDER_FUNCTION_TABLE providerAPITable =
        {
                .pfnDirectoryOpen            = &VmdirDbOpen,
                .pfnDirectoryBind            = &VmdirDbBind,
                .pfnDirectoryAdd             = &VmdirDbAddObject,
                .pfnDirectoryModify          = &VmdirDbModifyObject,
                .pfnDirectorySetPassword     = &VmdirDbSetPassword,
                .pfnDirectoryChangePassword  = &VmdirDbChangePassword,
                .pfnDirectoryVerifyPassword  = &VmdirDbVerifyPassword,
                .pfnDirectoryGetGroupMembers = &VmdirDbGetGroupMembers,
                .pfnDirectoryGetMemberships  = &VmdirDbGetUserMemberships,
                .pfnDirectoryAddToGroup      = &VmdirDbAddToGroup,
                .pfnDirectoryRemoveFromGroup = &VmdirDbRemoveFromGroup,
                .pfnDirectoryDelete          = &VmdirDbDeleteObject,
                .pfnDirectorySearch          = &VmdirDbSearchObject,
                .pfnDirectoryGetUserCount    = &VmdirDbGetUserCount,
                .pfnDirectoryGetGroupCount   = &VmdirDbGetGroupCount,
                .pfnDirectoryClose           = &VmdirDbClose
        };

    if (!ppszProviderName || !ppFnTable)
    {
        dwError = ERROR_INVALID_PARAMETER;
        BAIL_ON_VMDIRDB_ERROR(dwError);
    }

    gVmdirGlobals.pszProviderName = "Likewise vmdir LDAP provider Database";
    gVmdirGlobals.providerFunctionTable = providerAPITable;

    pthread_rwlock_init(&gVmdirGlobals.rwLock, NULL);
    gVmdirGlobals.pRwLock = &gVmdirGlobals.rwLock;

    dwError = VmDirGetBindProtocol(&gVmdirGlobals.bindProtocol);
    BAIL_ON_VMDIRDB_ERROR(dwError);

#if 1 /* For now, force SRP, as this will always work, and do not need to get a KRBTGT  */
    gVmdirGlobals.bindProtocol = VMDIR_BIND_PROTOCOL_SRP;
#endif

    /* TBD:Adam-Probably not needed */
    dwError = VmdirDbInit();
    BAIL_ON_VMDIRDB_ERROR(dwError);

#if 0 /* TBD:Adam-Move to Open */
    dwError = VmDirAllocLdapQueryMap(
        gVmdirGlobals.pszSearchBase
        &gVmdirGlobals.pLdapMap);
    BAIL_ON_VMDIRDB_ERROR(dwError);
#endif

    *ppszProviderName = gVmdirGlobals.pszProviderName;
    *ppFnTable = &gVmdirGlobals.providerFunctionTable;

cleanup:

    return dwError;

error:

    *ppszProviderName = NULL;
    *ppFnTable = NULL;

    goto cleanup;
}

DWORD
DirectoryShutdownProvider(
    PSTR pszProviderName,
    PDIRECTORY_PROVIDER_FUNCTION_TABLE pFnTable
    )
{
    DWORD dwError = 0;

    pthread_mutex_lock(&gVmdirGlobals.mutex);


    pthread_mutex_unlock(&gVmdirGlobals.mutex);


    return dwError;
}

DWORD
VmdirDbInit(
    VOID
    )
{
    DWORD dwError = 0;

    BAIL_ON_VMDIRDB_ERROR(dwError);
cleanup:

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
