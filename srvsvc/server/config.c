/* -*- mode: c; c-basic-offset: 4; indent-tabs-mode: nil; tab-width: 4 -*-
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * Editor Settings: expandtabs and use 4 spaces for indentation */

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
 * Copyright (C) Centeris Corporation 2004-2007
 * Copyright (C) Likewise Software 2007
 * All rights reserved.
 *
 * Authors: Sriram Nambakam (snambakam@likewisesoftware.com)
 *
 * Likewise Server Service (LWSRVSVC)
 *
 * Server Main
 *
 * Configuration
 *
 */

#include "includes.h"


static
DWORD
SrvsOpenConfig(
    PCSTR pszConfigKey,
    PCSTR pszPolicyKey,
    PSRVS_CONFIG_REG *ppReg
    );

static
VOID
SrvsCloseConfig(
    PSRVS_CONFIG_REG pReg
    );

static
DWORD
SrvsReadConfigString(
    PSRVS_CONFIG_REG pReg,
    PCSTR   pszName,
    BOOLEAN bUsePolicy,
    PSTR    *ppszValue,
    PDWORD  pdwSize
    );

static
DWORD
SrvsReadConfigDword(
    PSRVS_CONFIG_REG pReg,
    PCSTR pszName,
    BOOLEAN bUsePolicy,
    DWORD dwMin,
    DWORD dwMax,
    PDWORD pdwValue
    );

static
DWORD
SrvsReadConfigBoolean(
    PSRVS_CONFIG_REG pReg,
    PCSTR pszName,
    BOOLEAN bUsePolicy,
    PBOOLEAN pbValue
    );

DWORD
SrvsInitialiseConfig(
    PSRVSVC_CONFIG pConfig
    )
{
    DWORD err = ERROR_SUCCESS;

    memset(pConfig, 0, sizeof(*pConfig));

    strncpy(pConfig->szLpcSocketPath,
            SRVS_DEFAULT_LPC_SOCKET_PATH,
            (sizeof(pConfig->szLpcSocketPath)/
             sizeof(pConfig->szLpcSocketPath[0])) - 1);

    pConfig->RegisterTcpIp = FALSE;

    return err;
}

static
DWORD
SrvsOpenConfig(
    PCSTR pszConfigKey,
    PCSTR pszPolicyKey,
    PSRVS_CONFIG_REG *ppReg
    )
{
    DWORD err = LW_ERROR_SUCCESS;

    PSRVS_CONFIG_REG pReg = NULL;

    err = LwAllocateMemory(sizeof(SRVS_CONFIG_REG), (PVOID*)&pReg);
    BAIL_ON_SRVSVC_ERROR(err);

    err = LwAllocateString(pszConfigKey, &(pReg->pszConfigKey));
    BAIL_ON_SRVSVC_ERROR(err);

    err = LwAllocateString(pszPolicyKey, &(pReg->pszPolicyKey));
    BAIL_ON_SRVSVC_ERROR(err);

    err = RegOpenServer(&(pReg->hConnection));
    if (err || (pReg->hConnection == NULL))
    {
        err = ERROR_SUCCESS;
        goto error;
    }

    err = RegOpenKeyExA(
            pReg->hConnection,
            NULL,
            HKEY_THIS_MACHINE,
            0,
            KEY_READ,
            &(pReg->hKey));
    if (err)
    {
        err = ERROR_SUCCESS;
        goto error;
    }

error:
    if (err)
    {
        SrvsCloseConfig(pReg);
        pReg = NULL;
    }

    *ppReg = pReg;

    return err;
}

static
VOID
SrvsCloseConfig(
    PSRVS_CONFIG_REG pReg
    )
{
    if (pReg)
    {
        LW_SAFE_FREE_STRING(pReg->pszConfigKey);

        LW_SAFE_FREE_STRING(pReg->pszPolicyKey);
        if (pReg->hConnection)
        {
            if (pReg->hKey)
            {
                RegCloseKey(pReg->hConnection, pReg->hKey);
                pReg->hKey = NULL;
            }
            RegCloseServer(pReg->hConnection);
            pReg->hConnection = NULL;
        }

        LW_SAFE_FREE_MEMORY(pReg);
    }
}

static
DWORD
SrvsReadConfigString(
    PSRVS_CONFIG_REG pReg,
    PCSTR   pszName,
    BOOLEAN bUsePolicy,
    PSTR    *ppszValue,
    PDWORD  pdwSize
    )
{
    DWORD err = ERROR_SUCCESS;
    BOOLEAN bGotValue = FALSE;
    PSTR pszValue = NULL;
    char szValue[MAX_VALUE_LENGTH] = {0};
    DWORD dwType = 0;
    DWORD dwSize = 0;

    if (bUsePolicy)
    {
        dwSize = sizeof(szValue);
        memset(szValue, 0, dwSize);
        err = RegGetValueA(
                    pReg->hConnection,
                    pReg->hKey,
                    pReg->pszPolicyKey,
                    pszName,
                    RRF_RT_REG_SZ,
                    &dwType,
                    szValue,
                    &dwSize);
        if (!err)
        {
            bGotValue = TRUE;
        }
    }

    if (!bGotValue)
    {
        dwSize = sizeof(szValue);
        memset(szValue, 0, dwSize);
        err = RegGetValueA(
                    pReg->hConnection,
                    pReg->hKey,
                    pReg->pszConfigKey,
                    pszName,
                    RRF_RT_REG_SZ,
                    &dwType,
                    szValue,
                    &dwSize);
        if (!err)
        {
            bGotValue = TRUE;
        }
    }

    if (bGotValue)
    {
        err = LwAllocateString(szValue, &pszValue);
        BAIL_ON_SRVSVC_ERROR(err);

        LW_SAFE_FREE_STRING(*ppszValue);
        *ppszValue = pszValue;
        pszValue = NULL;

        if (pdwSize)
        {
            *pdwSize = dwSize;
        }
    }

    err = 0;

error:
    LW_SAFE_FREE_STRING(pszValue);

    return err;
}

static
DWORD
SrvsReadConfigDword(
    PSRVS_CONFIG_REG pReg,
    PCSTR pszName,
    BOOLEAN bUsePolicy,
    DWORD dwMin,
    DWORD dwMax,
    PDWORD pdwValue
    )
{
    DWORD err = ERROR_SUCCESS;
    BOOLEAN bGotValue = FALSE;
    DWORD dwValue;
    DWORD dwSize;
    DWORD dwType;

    if (bUsePolicy)
    {
        dwSize = sizeof(dwValue);
        err = RegGetValueA(
                    pReg->hConnection,
                    pReg->hKey,
                    pReg->pszPolicyKey,
                    pszName,
                    RRF_RT_REG_DWORD,
                    &dwType,
                    (PBYTE)&dwValue,
                    &dwSize);
        if (!err)
        {
            bGotValue = TRUE;
        }
    }

    if (!bGotValue)
    {
        dwSize = sizeof(dwValue);
        err = RegGetValueA(
                    pReg->hConnection,
                    pReg->hKey,
                    pReg->pszConfigKey,
                    pszName,
                    RRF_RT_REG_DWORD,
                    &dwType,
                    (PBYTE)&dwValue,
                    &dwSize);
        if (!err)
        {
            bGotValue = TRUE;
        }
    }

    if (bGotValue)
    {
        if ( dwMin <= dwValue && dwValue <= dwMax)
        {
            *pdwValue = dwValue;
        }
    }

    err = 0;

    return err;
}

static
DWORD
SrvsReadConfigBoolean(
    PSRVS_CONFIG_REG pReg,
    PCSTR pszName,
    BOOLEAN bUsePolicy,
    PBOOLEAN pbValue
    )
{

    DWORD err = ERROR_SUCCESS;
    DWORD value = *pbValue == TRUE ? 0x00000001 : 0x00000000;

    err = SrvsReadConfigDword(
                pReg,
                pszName,
                bUsePolicy,
                0,
                -1,
                &value);
    BAIL_ON_SRVSVC_ERROR(err);

    *pbValue = value ? TRUE : FALSE;

error:
    return err;
}

DWORD
SrvSvcReadConfigSettings(
    PSRVSVC_CONFIG pConfig
    )
{
    DWORD err = ERROR_SUCCESS;
    PSRVS_CONFIG_REG pReg = NULL;
    PSTR lpcSocketPath = NULL;

    err = SrvsOpenConfig(
              "Services\\srvsvc\\Parameters",
              "Policy\\Services\\srvsvc\\Parameters",
              &pReg);
    BAIL_ON_SRVSVC_ERROR(err);

    if (!pReg)
    {
        goto error;
    }

    err = SrvsReadConfigString(
              pReg,
              "LpcSocketPath",
              FALSE,
              &lpcSocketPath,
              NULL);
    BAIL_ON_SRVSVC_ERROR(err);

    strncpy(pConfig->szLpcSocketPath,
            lpcSocketPath,
            (sizeof(pConfig->szLpcSocketPath)/
             sizeof(pConfig->szLpcSocketPath[0])) - 1);

    err = SrvsReadConfigBoolean(
              pReg,
              "RegisterTcpIp",
              TRUE,
              &pConfig->RegisterTcpIp);
    BAIL_ON_SRVSVC_ERROR(err);

error:
    if (pReg)
    {
        SrvsCloseConfig(pReg);
    }

    LW_SAFE_FREE_MEMORY(lpcSocketPath);

    return err;
}

DWORD
SrvSvcConfigGetLpcSocketPath(
    PSTR* ppszPath
    )
{
    DWORD err = ERROR_SUCCESS;
    BOOLEAN bInLock = FALSE;
    PSTR path = NULL;

    SRVSVC_LOCK_MUTEX(bInLock, &gSrvsServerInfo.config.mutex);

    err = LwAllocateString(gSrvsServerInfo.config.szLpcSocketPath,
                           &path);
    BAIL_ON_SRVSVC_ERROR(err);

error:
    SRVSVC_UNLOCK_MUTEX(bInLock, &gSrvsServerInfo.config.mutex);

    if (err)
    {
        LW_SAFE_FREE_MEMORY(path);
    }

    *ppszPath = path;

    return err;
}

DWORD
SrvSvcConfigGetRegisterTcpIp(
    PBOOLEAN pRegisterTcpIp
    )
{
    DWORD err = ERROR_SUCCESS;
    BOOLEAN bInLock = FALSE;

    BAIL_ON_INVALID_PTR(pRegisterTcpIp, err);

    SRVSVC_LOCK_MUTEX(bInLock, &gSrvsServerInfo.config.mutex);

    *pRegisterTcpIp = gSrvsServerInfo.config.RegisterTcpIp;

error:
    SRVSVC_UNLOCK_MUTEX(bInLock, &gSrvsServerInfo.config.mutex);

    return err;
}
