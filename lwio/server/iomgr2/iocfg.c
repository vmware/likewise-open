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
 * Abstract:
 *
 */
#include "iop.h"

struct __SMB_CONFIG_REG
{
    HANDLE hConnection;
    HKEY hKey;
    wchar16_t *pwc16sConfigKey;
    wchar16_t *pwc16sPolicyKey;
};

DWORD
SMBProcessConfig(
    PCSTR pszConfigKey,
    PCSTR pszPolicyKey,
    PSMB_CONFIG_TABLE pConfig,
    DWORD dwConfigEntries
    )
{
    DWORD dwError = 0;
    DWORD dwEntry;

    PSMB_CONFIG_REG pReg = NULL;

    dwError = SMBOpenConfig(pszConfigKey, pszPolicyKey, &pReg);
    BAIL_ON_LWIO_ERROR(dwError);

    if ( pReg == NULL )
    {
        goto error;
    }

    for (dwEntry = 0; dwEntry < dwConfigEntries; dwEntry++)
    {
        dwError = 0;
        switch (pConfig[dwEntry].Type)
        {
            case SMBTypeString:
                dwError = SMBReadConfigString(
                            pReg,
                            pConfig[dwEntry].pszName,
                            pConfig[dwEntry].bUsePolicy,
                            pConfig[dwEntry].pValue);
                break;

            case SMBTypeDword:
                dwError = SMBReadConfigDword(
                            pReg,
                            pConfig[dwEntry].pszName,
                            pConfig[dwEntry].bUsePolicy,
                            pConfig[dwEntry].dwMin,
                            pConfig[dwEntry].dwMax,
                            pConfig[dwEntry].pValue);
                break;

            case SMBTypeBoolean:
                dwError = SMBReadConfigBoolean(
                            pReg,
                            pConfig[dwEntry].pszName,
                            pConfig[dwEntry].bUsePolicy,
                            pConfig[dwEntry].pValue);
                break;

            case SMBTypeEnum:
                dwError = SMBReadConfigEnum(
                            pReg,
                            pConfig[dwEntry].pszName,
                            pConfig[dwEntry].bUsePolicy,
                            pConfig[dwEntry].dwMin,
                            pConfig[dwEntry].dwMax,
                            pConfig[dwEntry].ppszEnumNames,
                            pConfig[dwEntry].pValue);
                break;

            default:
                break;
        }
        BAIL_ON_NON_LWREG_ERROR(dwError);
        dwError = 0;
    }

cleanup:
    SMBCloseConfig(pReg);
    pReg = NULL;

    return dwError;

error:
    goto cleanup;
}

DWORD
SMBOpenConfig(
    PCSTR pszConfigKey,
    PCSTR pszPolicyKey,
    PSMB_CONFIG_REG *ppReg
    )
{
    DWORD dwError = 0;

    PSMB_CONFIG_REG pReg = NULL;

    SMBAllocateMemory(sizeof(SMB_CONFIG_REG), (PVOID*)&pReg);
    BAIL_ON_LWIO_ERROR(dwError);

    dwError = SMBMbsToWc16s(pszConfigKey, &(pReg->pwc16sConfigKey));
    BAIL_ON_LWIO_ERROR(dwError);

    dwError = SMBMbsToWc16s(pszPolicyKey, &(pReg->pwc16sPolicyKey));
    BAIL_ON_LWIO_ERROR(dwError);

    dwError = RegOpenServer(&(pReg->hConnection));
    if ( dwError )
    {
        dwError = 0;
        goto error;
    }

    dwError = RegOpenRootKey(
                pReg->hConnection,
                LIKEWISE_ROOT_KEY,
                &(pReg->hKey));
    if (dwError)
    {
        dwError = 0;
        goto error;
    }

cleanup:

    *ppReg = pReg;

    return dwError;

error:

    SMBCloseConfig(pReg);
    pReg = NULL;

    goto cleanup;
}

VOID
SMBCloseConfig(
    PSMB_CONFIG_REG pReg
    )
{
    if ( pReg )
    {
        LWIO_SAFE_FREE_MEMORY(pReg->pwc16sConfigKey);

        LWIO_SAFE_FREE_MEMORY(pReg->pwc16sPolicyKey);
        if ( pReg->hConnection )
        {
            if ( pReg->hKey )
            {
                RegCloseKey(pReg->hConnection, pReg->hKey);
                pReg->hKey = NULL;
            }
            RegCloseServer(pReg->hConnection);
            pReg->hConnection = NULL;
        }

        LWIO_SAFE_FREE_MEMORY(pReg);
    }
}

DWORD
SMBReadConfigString(
    PSMB_CONFIG_REG pReg,
    PCSTR   pszName,
    BOOLEAN bUsePolicy,
    PSTR    *ppszValue
    )
{
    DWORD dwError = 0;

    wchar16_t *pwc16sName = NULL;

    BOOLEAN bGotValue = FALSE;
    PSTR pszValue = NULL;
    char szValue[MAX_VALUE_LENGTH];
    DWORD dwType;
    DWORD dwSize;

    dwError = SMBMbsToWc16s(pszName, &pwc16sName);
    BAIL_ON_LWIO_ERROR(dwError);

    if ( bUsePolicy )
    {
        dwSize = sizeof(szValue);
        memset(szValue, 0, dwSize);
        dwError = RegGetValue(
                    pReg->hConnection,
                    pReg->hKey,
                    pReg->pwc16sPolicyKey,
                    pwc16sName,
                    RRF_RT_REG_SZ,
                    &dwType,
                    szValue,
                    &dwSize);
        if (!dwError)
            bGotValue = TRUE;
    }

    if (!bGotValue )
    {
        dwSize = sizeof(szValue);
        memset(szValue, 0, dwSize);
        dwError = RegGetValue(
                    pReg->hConnection,
                    pReg->hKey,
                    pReg->pwc16sConfigKey,
                    pwc16sName,
                    RRF_RT_REG_SZ,
                    &dwType,
                    szValue,
                    &dwSize);
        if (!dwError)
            bGotValue = TRUE;
    }

    if (bGotValue)
    {
        dwError = SMBAllocateString(szValue, &pszValue);
        BAIL_ON_LWIO_ERROR(dwError);

        LWIO_SAFE_FREE_STRING(*ppszValue);
        *ppszValue = pszValue;
        pszValue = NULL;
    }

    dwError = 0;

cleanup:
    LWIO_SAFE_FREE_MEMORY(pwc16sName);

    LWIO_SAFE_FREE_MEMORY(pszValue);

    return dwError;

error:
    goto cleanup;
}

DWORD
SMBReadConfigDword(
    PSMB_CONFIG_REG pReg,
    PCSTR pszName,
    BOOLEAN bUsePolicy,
    DWORD dwMax,
    DWORD dwMin,
    PDWORD pdwValue
    )
{
    DWORD dwError = 0;

    wchar16_t *pwc16sName = NULL;

    BOOLEAN bGotValue = FALSE;
    DWORD dwValue;
    DWORD dwSize;
    DWORD dwType;

    dwError = SMBMbsToWc16s(pszName, &pwc16sName);
    BAIL_ON_LWIO_ERROR(dwError);

    if (bUsePolicy)
    {
        dwSize = sizeof(dwValue);
        dwError = RegGetValue(
                    pReg->hConnection,
                    pReg->hKey,
                    pReg->pwc16sPolicyKey,
                    pwc16sName,
                    RRF_RT_REG_DWORD,
                    &dwType,
                    (PBYTE)&dwValue,
                    &dwSize);
        if (!dwError)
        {
            bGotValue = TRUE;
        }
    }

    if (!bGotValue)
    {
        dwSize = sizeof(dwValue);
        dwError = RegGetValue(
                    pReg->hConnection,
                    pReg->hKey,
                    pReg->pwc16sConfigKey,
                    pwc16sName,
                    RRF_RT_REG_DWORD,
                    &dwType,
                    (PBYTE)&dwValue,
                    &dwSize);
        if (!dwError)
        {
            bGotValue = TRUE;
        }
    }

    if (bGotValue)
    {
        if ( dwMin <= dwValue && dwValue <= dwMax)
            *pdwValue = dwValue;
    }

    dwError = 0;

cleanup:
    LWIO_SAFE_FREE_MEMORY(pwc16sName);

    return dwError;

error:
    goto cleanup;
}

DWORD
SMBReadConfigBoolean(
    PSMB_CONFIG_REG pReg,
    PCSTR pszName,
    BOOLEAN bUsePolicy,
    PBOOLEAN pbValue
    )
{

    DWORD dwError = 0;

    DWORD dwValue = *pbValue == TRUE ? 0x00000001 : 0x00000000;

    dwError = SMBReadConfigDword(
                pReg,
                pszName,
                bUsePolicy,
                0,
                -1,
                &dwValue);
    BAIL_ON_LWIO_ERROR(dwError);

    *pbValue = dwValue ? TRUE : FALSE;

cleanup:

    return dwError;

error:
    goto cleanup;
}

DWORD
SMBReadConfigEnum(
    PSMB_CONFIG_REG pReg,
    PCSTR   pszName,
    BOOLEAN bUsePolicy,
    DWORD   dwMin,
    DWORD   dwMax,
    const PCSTR   *ppszEnumNames,
    PDWORD  pdwValue
    )
{
    DWORD dwError = 0;
    PSTR pszValue = NULL;

    DWORD dwEnumIndex;

    dwError = SMBReadConfigString(
                pReg,
                pszName,
                bUsePolicy,
                &pszValue);
    BAIL_ON_LWIO_ERROR(dwError);

    if (pszValue != NULL )
    {
        for (dwEnumIndex = 0;
             dwEnumIndex <= dwMax - dwMin;
             dwEnumIndex++)
        {
            if(!strcasecmp(pszValue, ppszEnumNames[dwEnumIndex]))
            {
                *pdwValue = dwEnumIndex + dwMin;
                goto cleanup;
            }
        }
    }

cleanup:
    LWIO_SAFE_FREE_STRING(pszValue);
    return dwError;

error:
    goto cleanup;
}

