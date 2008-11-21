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
 *        provider-main.c
 *
 * Abstract:
 *
 *        Likewise Security and Authentication Subsystem (LSASS)
 *
 *        Local Authentication Provider
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 *          Wei Fu (wfu@likewisesoftware.com)
 *          Brian Dunstan (bdunstan@likewisesoftware.com)
 *          Kyle Stemen (kstemen@likewisesoftware.com)
 */
#include "adprovider.h"

DWORD
UnprovisionedModeEnumUsers(
    HANDLE  hProvider,
    PCSTR   pszDomainDnsName,
    PAD_ENUM_STATE pEnumState,
    DWORD   dwMaxNumUsers,
    PDWORD  pdwNumUsersFound,
    PVOID** pppUserInfoList
    )
{
    DWORD dwError = 0;

    dwError = DefaultModeSchemaOrUnprovisionEnumObjects(
                   LSA_AD_ENUM_TYPE_USER,
                   UNPROVISIONED_MODE,
                   ADMarshalUserInfoList,
                   LsaFreeUserInfoList,
                   pszDomainDnsName,
                   pEnumState,
                   dwMaxNumUsers,
                   pdwNumUsersFound,
                   pppUserInfoList);
    BAIL_ON_LSA_ERROR(dwError);

cleanup:

    return dwError;

error:
    if (*pppUserInfoList)
    {
        LsaFreeUserInfoList(pEnumState->dwInfoLevel, *pppUserInfoList, *pdwNumUsersFound);
    }
    *pdwNumUsersFound = 0;
    *pppUserInfoList = NULL;

    goto cleanup;
}

DWORD
UnprovisionedModeEnumGroups(
    HANDLE  hProvider,
    PCSTR   pszDomainDnsName,
    PAD_ENUM_STATE pEnumState,
    DWORD   dwMaxNumGroups,
    PDWORD  pdwNumGroupsFound,
    PVOID** pppGroupInfoList
    )
{
    DWORD dwError = 0;

    dwError = DefaultModeSchemaOrUnprovisionEnumObjects(
                   LSA_AD_ENUM_TYPE_GROUP,
                   UNPROVISIONED_MODE,
                   ADMarshalGroupInfoList,
                   LsaFreeGroupInfoList,
                   pszDomainDnsName,
                   pEnumState,
                   dwMaxNumGroups,
                   pdwNumGroupsFound,
                   pppGroupInfoList);
    BAIL_ON_LSA_ERROR(dwError);

cleanup:

    return dwError;

error:
    if (*pppGroupInfoList)
    {
        LsaFreeUserInfoList(pEnumState->dwInfoLevel, *pppGroupInfoList, *pdwNumGroupsFound);
    }
    *pdwNumGroupsFound = 0;
    *pppGroupInfoList = NULL;

    goto cleanup;
}
