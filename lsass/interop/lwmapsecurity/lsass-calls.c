/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 */

/*
 * Copyright Likewise Software. All rights reserved.
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
 *        lsass-calls.c
 *
 * Abstract:
 *
 *        Likewise Security and Authentication Subsystem (LSASS)
 *
 *        LSASS API calls for internal and public plugin
 *
 * Authors: Danilo Almeida (dalmeida@likewise.com)
 *          Rafal Szczesniak (rafal@likewise.com)
 */

#include <lw/base.h>
#include <lwmem.h>
#include <lsa/lsa.h>
#include "lsass-calls.h"


/*
 * The idea behind the macro magic is to have essentially the same code
 * creating access tokens available both inside and outside lsass.
 * Substituting function calls with server side calls (LsaSrv*) for
 * internal plugin and with client side calls (Lsa*) for public plugin
 * enables using lwmapsecurity inside lsass without having a server
 * calling itself via client calls.
 * The exceptions are LsaGetNamesBySidList and LsaCloseServer functions
 * which are slightly different in client and server APIs, so they need
 * a wrapper implementation.
 */

#ifdef LSASS_INTERNAL_PLUGIN

DWORD
LsaGetNamesBySidList(
    IN  HANDLE         hConnection,
    IN  size_t         sCount,
    IN  PSTR*          ppszSidList,
    OUT PLSA_SID_INFO* ppSIDInfoList,
    OUT OPTIONAL CHAR *pchDomainSeparator
    )
{
    DWORD dwError = 0;
    PSTR *ppszDomainNames = NULL;
    PSTR *ppszSamAccountNames = NULL;
    ADAccountType *pTypes = NULL;
    PLSA_SID_INFO pSidInfo = NULL;
    DWORD i = 0;

    dwError = LsaSrvGetNamesBySidList(hConnection,
                                      sCount,
                                      ppszSidList,
                                      &ppszDomainNames,
                                      &ppszSamAccountNames,
                                      &pTypes,
                                      pchDomainSeparator);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LwAllocateMemory(sizeof(*pSidInfo) * sCount,
                               OUT_PPVOID(&pSidInfo));
    BAIL_ON_LSA_ERROR(dwError);

    for (i = 0; i < sCount; i++)
    {
        pSidInfo[i].accountType       = pTypes[i];
        pSidInfo[i].pszSamAccountName = ppszSamAccountNames[i];
        pSidInfo[i].pszDomainName     = ppszDomainNames[i];
    }

    *ppSIDInfoList = pSidInfo;

cleanup:
    return dwError;

error:
    *ppSIDInfoList      = NULL;
    *pchDomainSeparator = '\0';

    goto cleanup;
}


DWORD
LsaCloseServer(
    HANDLE hConnection
    )
{
    DWORD dwError = 0;

    LsaSrvCloseServer(hConnection);

    return dwError;
}


#endif /* LSASS_INTERNAL_PLUGIN */


/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
