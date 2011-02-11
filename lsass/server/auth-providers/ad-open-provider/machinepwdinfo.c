/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 */

/*
 * Copyright (c) Likewise Software.  All rights reserved.
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
 * license@likewise.com
 */

/*
 * Copyright (C) Likewise Software. All rights reserved.
 *
 * Module Name:
 *
 *        machinepwdinfo.c
 *
 * Abstract:
 *
 *        LSASS AD Provider
 *
 *        Machine Account/Password Info
 *
 * Authors: Danilo Almeida <dalmeida@likewise.com>
 *
 */

#include "adprovider.h"
#include <lsa/lsapstore-api.h>
#include <lwtime.h>

DWORD
AD_GetMachineAccountInfoA(
    OUT PLSA_MACHINE_ACCOUNT_INFO_A* ppAccountInfo
    )
{
    DWORD dwError = 0;
    PLSA_MACHINE_PASSWORD_INFO_A pPasswordInfo = NULL;

    dwError = AD_GetMachinePasswordInfoA(
                    &pPasswordInfo);
    BAIL_ON_LSA_ERROR(dwError);

    LW_SECURE_FREE_STRING(pPasswordInfo->Password);

    *ppAccountInfo = &pPasswordInfo->Account;

cleanup:
    return dwError;

error:
    *ppAccountInfo = NULL;
    LsaPstoreFreePasswordInfoA(pPasswordInfo);
    goto cleanup;
}

DWORD
AD_GetMachineAccountInfoW(
    OUT PLSA_MACHINE_ACCOUNT_INFO_W* ppAccountInfo
    )
{
    DWORD dwError = 0;
    PLSA_MACHINE_PASSWORD_INFO_W pPasswordInfo = NULL;

    dwError = AD_GetMachinePasswordInfoW(
                    &pPasswordInfo);
    BAIL_ON_LSA_ERROR(dwError);

    LW_SECURE_FREE_WSTRING(pPasswordInfo->Password);

    *ppAccountInfo = &pPasswordInfo->Account;

cleanup:
    return dwError;

error:
    *ppAccountInfo = NULL;
    LsaPstoreFreePasswordInfoW(pPasswordInfo);
    goto cleanup;
}

DWORD
AD_GetMachinePasswordInfoA(
    OUT PLSA_MACHINE_PASSWORD_INFO_A* ppPasswordInfo
    )
{
    DWORD dwError = 0;
    PLSA_MACHINE_PASSWORD_INFO_W pPasswordInfoW = NULL;
    PLSA_MACHINE_PASSWORD_INFO_A pPasswordInfoA = NULL;

    dwError = AD_GetMachinePasswordInfoW(
                    &pPasswordInfoW);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaPstoreConvertWideToAnsiPasswordInfo(
                    pPasswordInfoW,
                    &pPasswordInfoA);
    BAIL_ON_LSA_ERROR(dwError);

    *ppPasswordInfo = pPasswordInfoA;

cleanup:
    return dwError;

error:
    *ppPasswordInfo = NULL;
    LsaPstoreFreePasswordInfoA(pPasswordInfoA);
    goto cleanup;
}

DWORD
AD_GetMachinePasswordInfoW(
    OUT PLSA_MACHINE_PASSWORD_INFO_W* ppPasswordInfo
    )
{
    DWORD dwError = 0;
    PLWPS_PASSWORD_INFO pInfo = NULL;
    HANDLE hStore = NULL;
    PLSA_MACHINE_PASSWORD_INFO_W pPasswordInfo = NULL;

    dwError = LwpsOpenPasswordStore(
                LWPS_PASSWORD_STORE_DEFAULT,
                &hStore);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LwpsGetPasswordByCurrentHostName(
                hStore,
                &pInfo);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LwAllocateMemory(
                    sizeof(*pPasswordInfo),
                    OUT_PPVOID(&pPasswordInfo));
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LwAllocateWc16String(
                    &pPasswordInfo->Account.DnsDomainName,
                    pInfo->pwszDnsDomainName);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LwAllocateWc16String(
                    &pPasswordInfo->Account.NetbiosDomainName,
                    pInfo->pwszDomainName);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LwAllocateWc16String(
                    &pPasswordInfo->Account.DomainSid,
                    pInfo->pwszSID);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LwAllocateWc16String(
                    &pPasswordInfo->Account.SamAccountName,
                    pInfo->pwszMachineAccount);
    BAIL_ON_LSA_ERROR(dwError);

    switch(pInfo->dwSchannelType)
    {
        default:
        case 2:
            pPasswordInfo->Account.AccountFlags =
                LSA_MACHINE_ACCOUNT_TYPE_WORKSTATION;
            break;
        case 4:
            pPasswordInfo->Account.AccountFlags = LSA_MACHINE_ACCOUNT_TYPE_DC;
            break;
        case 6:
            pPasswordInfo->Account.AccountFlags = LSA_MACHINE_ACCOUNT_TYPE_BDC;
            break;
    }

    pPasswordInfo->Account.KeyVersionNumber = 0;

    dwError = LwAllocateWc16String(
                    &pPasswordInfo->Account.Fqdn,
                    pInfo->pwszHostDnsDomain);
    BAIL_ON_LSA_ERROR(dwError);

    pPasswordInfo->Account.LastChangeTime =
        LwWinTimeToNtTime(pInfo->last_change_time);

    dwError = LwAllocateWc16String(
                    &pPasswordInfo->Password,
                    pInfo->pwszMachinePassword);
    BAIL_ON_LSA_ERROR(dwError);

    *ppPasswordInfo = pPasswordInfo;

cleanup:
    if (pInfo)
    {
        LwpsFreePasswordInfo(hStore, pInfo);
    }
    if (hStore)
    {
        LwpsClosePasswordStore(hStore);
    }
    return dwError;

error:
    *ppPasswordInfo = NULL;
    LsaPstoreFreePasswordInfoW(pPasswordInfo);
    goto cleanup;
}
