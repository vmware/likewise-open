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
 * Authors: Scott Salley <ssalley@likewise.com>
 *
 */

#include "includes.h"

struct __LWIO_CONFIG_REG
{
    HANDLE hConnection;
    HKEY hKey;
    PSTR pszConfigKey;
    PSTR pszPolicyKey;
};

/**
 * Read configuration values from the registry
 *
 * This function loops through a configuration table reading all given values
 * from a registry key.  If an entry is not found in the registry
 * @dwConfigEntries determines whether an error is returned.
 *
 * @param[in] pszConfigKey Registry key path
 * @param[in] pszPolicyKey Registry policy key path
 * @param[in] pConfig Configuration table specifying parameter names
 * @param[in] dwConfigEntries Number of table entries
 * @param[in] bIgnoreNotFound Don't error if a parameter is not found in the
 *                            registry.
 *
 * @return STATUS_SUCCESS, or appropriate error.
 */
NTSTATUS
LwIoProcessConfig(
    PCSTR pszConfigKey,
    PCSTR pszPolicyKey,
    PLWIO_CONFIG_TABLE pConfig,
    DWORD dwConfigEntries,
    BOOLEAN bIgnoreNotFound
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    DWORD dwEntry = 0;
    PLWIO_CONFIG_REG pReg = NULL;
    PSTR pszTmp = NULL;
    PSTR *ppszTmp = NULL;

    ntStatus = LwIoOpenConfig(pszConfigKey, pszPolicyKey, &pReg);
    BAIL_ON_NT_STATUS(ntStatus);

    if (pReg == NULL)
    {
        goto error;
    }

    for (dwEntry = 0; dwEntry < dwConfigEntries; dwEntry++)
    {
        ntStatus = STATUS_SUCCESS;
        switch (pConfig[dwEntry].Type)
        {
            case LwIoTypeString:
                ntStatus = LwIoReadConfigString(
                            pReg,
                            pConfig[dwEntry].pszName,
                            pConfig[dwEntry].bUsePolicy,
                            &pszTmp);
                if (ntStatus == STATUS_SUCCESS)
                {
                    /* Free the old string, if it existed. */
                    LwRtlCStringFree((PSTR *)pConfig[dwEntry].pValue);
                    *((PSTR *)pConfig[dwEntry].pValue) = pszTmp;
                }
                break;

            case LwIoTypeMultiString:
                ntStatus = LwIoReadConfigMultiString(
                            pReg,
                            pConfig[dwEntry].pszName,
                            pConfig[dwEntry].bUsePolicy,
                            &ppszTmp);
                if (ntStatus == STATUS_SUCCESS)
                {
                    LwIoMultiStringFree(*((PSTR **)pConfig[dwEntry].pValue));
                    *((PSTR **)pConfig[dwEntry].pValue) = ppszTmp;
                }
                break;

            case LwIoTypeDword:
                ntStatus = LwIoReadConfigDword(
                            pReg,
                            pConfig[dwEntry].pszName,
                            pConfig[dwEntry].bUsePolicy,
                            pConfig[dwEntry].dwMin,
                            pConfig[dwEntry].dwMax,
                            pConfig[dwEntry].pValue);
                break;

            case LwIoTypeBoolean:
                ntStatus = LwIoReadConfigBoolean(
                            pReg,
                            pConfig[dwEntry].pszName,
                            pConfig[dwEntry].bUsePolicy,
                            pConfig[dwEntry].pValue);
                break;

            case LwIoTypeEnum:
                ntStatus = LwIoReadConfigEnum(
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
        if (bIgnoreNotFound && ntStatus == STATUS_OBJECT_NAME_NOT_FOUND)
        {
            ntStatus = STATUS_SUCCESS;
        }
        BAIL_ON_NT_STATUS(ntStatus);
    }

cleanup:
    LwIoCloseConfig(pReg);
    pReg = NULL;

    return ntStatus;

error:
    goto cleanup;
}

NTSTATUS
LwIoOpenConfig(
    PCSTR pszConfigKey,
    PCSTR pszPolicyKey,
    PLWIO_CONFIG_REG *ppReg
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PLWIO_CONFIG_REG pReg = NULL;

    ntStatus = LW_RTL_ALLOCATE(
                   (PVOID*)&pReg,
                   LWIO_CONFIG_REG,
                   sizeof(LWIO_CONFIG_REG));
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = LwRtlCStringDuplicate(&pReg->pszConfigKey, pszConfigKey);
    BAIL_ON_NT_STATUS(ntStatus);

    if (pszPolicyKey)
    {
        ntStatus = LwRtlCStringDuplicate(&pReg->pszPolicyKey, pszPolicyKey);
        BAIL_ON_NT_STATUS(ntStatus);
    }

    ntStatus = NtRegOpenServer(&pReg->hConnection);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = NtRegOpenKeyExA(
            pReg->hConnection,
            NULL,
            HKEY_THIS_MACHINE,
            0,
            KEY_READ,
            &(pReg->hKey));
    BAIL_ON_NT_STATUS(ntStatus);

cleanup:

    *ppReg = pReg;

    return ntStatus;

error:

    LwIoCloseConfig(pReg);
    pReg = NULL;

    goto cleanup;
}

VOID
LwIoCloseConfig(
    PLWIO_CONFIG_REG pReg
    )
{
    if (pReg)
    {
        LwRtlCStringFree(&pReg->pszConfigKey);

        LwRtlCStringFree(&pReg->pszPolicyKey);

        if (pReg->hConnection)
        {
            if ( pReg->hKey )
            {
                NtRegCloseKey(pReg->hConnection, pReg->hKey);
                pReg->hKey = NULL;
            }
            NtRegCloseServer(pReg->hConnection);
            pReg->hConnection = NULL;
        }

        RTL_FREE(&pReg);
    }
}

NTSTATUS
LwIoReadConfigString(
    PLWIO_CONFIG_REG pReg,
    PCSTR   pszName,
    BOOLEAN bUsePolicy,
    PSTR    *ppszValue
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    BOOLEAN bGotValue = FALSE;
    char szValue[MAX_VALUE_LENGTH];
    DWORD dwType = 0;
    DWORD dwSize = 0;

    if (bUsePolicy)
    {
        if (!pReg->pszPolicyKey)
        {
            ntStatus = STATUS_INVALID_PARAMETER;
            BAIL_ON_NT_STATUS(ntStatus);
        }

        dwSize = sizeof(szValue);
        memset(szValue, 0, dwSize);
        ntStatus = NtRegGetValueA(
                    pReg->hConnection,
                    pReg->hKey,
                    pReg->pszPolicyKey,
                    pszName,
                    RRF_RT_REG_SZ,
                    &dwType,
                    szValue,
                    &dwSize);
        if (!ntStatus)
        {
            bGotValue = TRUE;
        }
    }

    if (!bGotValue)
    {
        dwSize = sizeof(szValue);
        memset(szValue, 0, dwSize);
        ntStatus = NtRegGetValueA(
                    pReg->hConnection,
                    pReg->hKey,
                    pReg->pszConfigKey,
                    pszName,
                    RRF_RT_REG_SZ,
                    &dwType,
                    szValue,
                    &dwSize);
        if (!ntStatus)
        {
            bGotValue = TRUE;
        }
    }

    if (bGotValue)
    {
        ntStatus = LwRtlCStringDuplicate(ppszValue, szValue);
        BAIL_ON_NT_STATUS(ntStatus);
    }

cleanup:
    return ntStatus;

error:
    goto cleanup;
}

NTSTATUS
LwIoReadConfigMultiString(
    PLWIO_CONFIG_REG pReg,
    PCSTR   pszName,
    BOOLEAN bUsePolicy,
    PSTR    **pppszValue
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    BOOLEAN bGotValue = FALSE;
    BYTE szValue[MAX_VALUE_LENGTH];
    PSTR *ppszValue = NULL;
    DWORD dwType = 0;
    DWORD dwSize = 0;

    if (bUsePolicy)
    {
        if (!pReg->pszPolicyKey)
        {
            ntStatus = STATUS_INVALID_PARAMETER;
            BAIL_ON_NT_STATUS(ntStatus);
        }

        dwSize = sizeof(szValue);
        memset(szValue, 0, dwSize);
        ntStatus = NtRegGetValueA(
                    pReg->hConnection,
                    pReg->hKey,
                    pReg->pszPolicyKey,
                    pszName,
                    RRF_RT_REG_MULTI_SZ,
                    &dwType,
                    szValue,
                    &dwSize);
        if (!ntStatus)
        {
            bGotValue = TRUE;
        }
    }

    if (!bGotValue)
    {
        dwSize = sizeof(szValue);
        memset(szValue, 0, dwSize);
        ntStatus = NtRegGetValueA(
                    pReg->hConnection,
                    pReg->hKey,
                    pReg->pszConfigKey,
                    pszName,
                    RRF_RT_REG_MULTI_SZ,
                    &dwType,
                    szValue,
                    &dwSize);
        if (!ntStatus)
        {
            bGotValue = TRUE;
        }
    }

    if (bGotValue)
    {
        ntStatus = NtRegByteArrayToMultiStrsA(szValue, dwSize, &ppszValue);
        BAIL_ON_NT_STATUS(ntStatus);
        *pppszValue = ppszValue;
    }

cleanup:
    return ntStatus;

error:
    goto cleanup;
}

VOID
LwIoMultiStringFree(
    PSTR *ppszStrings
    )
{
    RegFreeMultiStrsA(ppszStrings);
}

NTSTATUS
LwIoReadConfigDword(
    PLWIO_CONFIG_REG pReg,
    PCSTR pszName,
    BOOLEAN bUsePolicy,
    DWORD dwMin,
    DWORD dwMax,
    PDWORD pdwValue
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    BOOLEAN bGotValue = FALSE;
    DWORD dwValue = 0;
    DWORD dwSize =0;
    DWORD dwType = 0;

    if (bUsePolicy)
    {
        if (!pReg->pszPolicyKey)
        {
            ntStatus = STATUS_INVALID_PARAMETER;
            BAIL_ON_NT_STATUS(ntStatus);
        }

        dwSize = sizeof(dwValue);
        ntStatus = NtRegGetValueA(
                    pReg->hConnection,
                    pReg->hKey,
                    pReg->pszPolicyKey,
                    pszName,
                    RRF_RT_REG_DWORD,
                    &dwType,
                    (PBYTE)&dwValue,
                    &dwSize);
        if (!ntStatus)
        {
            bGotValue = TRUE;
        }
    }

    if (!bGotValue)
    {
        dwSize = sizeof(dwValue);
        ntStatus = NtRegGetValueA(
                    pReg->hConnection,
                    pReg->hKey,
                    pReg->pszConfigKey,
                    pszName,
                    RRF_RT_REG_DWORD,
                    &dwType,
                    (PBYTE)&dwValue,
                    &dwSize);
        if (!ntStatus)
        {
            bGotValue = TRUE;
        }
    }

    if (bGotValue)
    {
        if (dwMin <= dwValue && dwValue <= dwMax)
        {
            *pdwValue = dwValue;
        }
        else
        {
            ntStatus = STATUS_INVALID_PARAMETER;
        }
    }

cleanup:
    return ntStatus;

error:
    goto cleanup;
}

NTSTATUS
LwIoReadConfigBoolean(
    PLWIO_CONFIG_REG pReg,
    PCSTR pszName,
    BOOLEAN bUsePolicy,
    PBOOLEAN pbValue
    )
{

    NTSTATUS ntStatus = STATUS_SUCCESS;
    DWORD dwValue = *pbValue == TRUE ? 0x00000001 : 0x00000000;

    ntStatus = LwIoReadConfigDword(
                pReg,
                pszName,
                bUsePolicy,
                0,
                -1,
                &dwValue);
    BAIL_ON_NT_STATUS(ntStatus);

    *pbValue = dwValue ? TRUE : FALSE;

cleanup:

    return ntStatus;

error:
    goto cleanup;
}

NTSTATUS
LwIoReadConfigEnum(
    PLWIO_CONFIG_REG pReg,
    PCSTR   pszName,
    BOOLEAN bUsePolicy,
    DWORD   dwMin,
    DWORD   dwMax,
    const PCSTR   *ppszEnumNames,
    PDWORD  pdwValue
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PSTR pszValue = NULL;
    DWORD dwEnumIndex = 0;

    ntStatus = LwIoReadConfigString(
                pReg,
                pszName,
                bUsePolicy,
                &pszValue);
    BAIL_ON_NT_STATUS(ntStatus);

    if (pszValue != NULL )
    {
        for (dwEnumIndex = 0;
             dwEnumIndex <= dwMax - dwMin;
             dwEnumIndex++)
        {
            if(LwRtlCStringCompare(
                   pszValue,
                   ppszEnumNames[dwEnumIndex], FALSE) == 0)
            {
                *pdwValue = dwEnumIndex + dwMin;
                break;
            }
        }
    }

cleanup:
    LwRtlCStringFree(&pszValue);

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
