/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

/*
 * Copyright Likewise Software    2004-2008
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
 *        lpmisc.c
 *
 * Abstract:
 *
 *        Likewise Security and Authentication Subsystem (LSASS)
 *
 *        Local Authentication Provider
 *
 *        Miscellaneous
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 */

#include "includes.h"


DWORD
LocalBuildDN(
    PLSA_LOGIN_NAME_INFO pLoginInfo,
    PWSTR*               ppwszDN
    )
{
    DWORD dwError = 0;
    WCHAR wszCNPrefix[] = LOCAL_DIR_CN_PREFIX;
    PWSTR pwszName = NULL;
    PWSTR pwszDN = NULL;

    dwError = LsaMbsToWc16s(
                    pLoginInfo->pszName,
                    &pwszName);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaAllocateMemory(
                    sizeof(wszCNPrefix) + strlen(pLoginInfo->pszName) * sizeof(WCHAR),
                    (PVOID*)&pwszDN);
    BAIL_ON_LSA_ERROR(dwError);

    // Build CN=<sam account name>
    memcpy((PBYTE)pwszDN, (PBYTE)&wszCNPrefix[0], sizeof(wszCNPrefix) - sizeof(WCHAR));

    memcpy((PBYTE)(pwszDN + sizeof(wszCNPrefix) - sizeof(WCHAR)),
           (PBYTE)pwszName,
           strlen(pLoginInfo->pszName) * sizeof(WCHAR));

    *ppwszDN = pwszDN;

cleanup:

    LSA_SAFE_FREE_MEMORY(pwszName);

    return dwError;

error:

    *ppwszDN = NULL;

    LSA_SAFE_FREE_MEMORY(pwszDN);

    goto cleanup;
}

LONG64
LocalGetNTTime(
    time_t timeVal
    )
{
    return (timeVal + 11644473600LL) * 10000000LL;
}
