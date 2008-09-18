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
 *        localcfg.c
 *
 * Abstract:
 *
 *        Likewise Security and Authentication Subsystem (LSASS)
 * 
 *        Local Authentication Provider
 * 
 *        Wrappers for accessing global configuration variables
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 *          Brian Dunstan (bdunstan@likewisesoftware.com)
 */
#include "localprovider.h"

DWORD
LsaProviderLocal_SetConfigFilePath(
    PCSTR pszConfigFilePath
    )
{
    DWORD dwError = 0;
    BOOLEAN bInLock = FALSE;
    PSTR pszConfigFilePathLocal = NULL;

    BAIL_ON_INVALID_STRING(pszConfigFilePath);

    dwError = LsaAllocateString(
                    pszConfigFilePath,
                    &pszConfigFilePathLocal
                    );
    BAIL_ON_LSA_ERROR(dwError);

    ENTER_LOCAL_GLOBAL_DATA_RW_WRITER_LOCK(bInLock);
 
    LSA_SAFE_FREE_STRING(gpszConfigFilePath);
    
    gpszConfigFilePath = pszConfigFilePathLocal;

cleanup:

    LEAVE_LOCAL_GLOBAL_DATA_RW_WRITER_LOCK(bInLock);

    return dwError;

error:

    LSA_SAFE_FREE_STRING(pszConfigFilePathLocal);
   
    goto cleanup;
}

DWORD
LsaProviderLocal_GetConfigFilePath(
    PSTR* ppszConfigFilePath
    )
{
    DWORD dwError = 0;
    BOOLEAN bInLock = FALSE;
    PSTR pszConfigFilePath = NULL;

    ENTER_LOCAL_GLOBAL_DATA_RW_READER_LOCK(bInLock);

    if (!IsNullOrEmptyString(gpszConfigFilePath))
    {
        dwError = LsaAllocateString(
                        gpszConfigFilePath,
                        &pszConfigFilePath
                        );
        BAIL_ON_LSA_ERROR(dwError);
    }

    *ppszConfigFilePath = pszConfigFilePath;

cleanup:

    LEAVE_LOCAL_GLOBAL_DATA_RW_READER_LOCK(bInLock);

    return dwError;

error:

    *ppszConfigFilePath = NULL;

    goto cleanup;
}

DWORD
LsaProviderLocal_SetPasswdChangeInterval(
    DWORD dwPasswdChangeInterval
    )
{
    DWORD dwError = 0;
    BOOLEAN bInLock = FALSE;

    if (dwPasswdChangeInterval < gProviderLocal_PasswdChangeIntervalMinimum)
    {
        LSA_LOG_ERROR("Failed to set PasswdChangeInterval to %u.  Minimum is %u.",
                        dwPasswdChangeInterval,
                        gProviderLocal_PasswdChangeIntervalMinimum
                        );
        dwError = LSA_ERROR_INVALID_PARAMETER;
    }

    if (dwPasswdChangeInterval > gProviderLocal_PasswdChangeIntervalMaximum)
    {
        LSA_LOG_ERROR("Failed to set PasswdChangeInterval to %u.  Maximum is %u.",
                        dwPasswdChangeInterval,
                        gProviderLocal_PasswdChangeIntervalMaximum
                        );
        dwError = LSA_ERROR_INVALID_PARAMETER;
    }
    BAIL_ON_LSA_ERROR(dwError);

    ENTER_LOCAL_GLOBAL_DATA_RW_WRITER_LOCK(bInLock);
 
    gProviderLocal_PasswdChangeInterval = dwPasswdChangeInterval;

cleanup:

    LEAVE_LOCAL_GLOBAL_DATA_RW_WRITER_LOCK(bInLock);

    return dwError;

error:

    goto cleanup;
}

DWORD
LsaProviderLocal_GetPasswdChangeInterval(
    VOID
    )
{
    DWORD dwPasswdChangeInterval = 0;
    BOOLEAN bInLock = FALSE;

    ENTER_LOCAL_GLOBAL_DATA_RW_READER_LOCK(bInLock);
    dwPasswdChangeInterval = gProviderLocal_PasswdChangeInterval;
    LEAVE_LOCAL_GLOBAL_DATA_RW_READER_LOCK(bInLock);

    return dwPasswdChangeInterval;
}

DWORD
LsaProviderLocal_SetPasswdChangeWarningTime(
    DWORD dwPasswdChangeWarningTime
    )
{
    DWORD dwError = 0;
    BOOLEAN bInLock = FALSE;

    if (dwPasswdChangeWarningTime < gProviderLocal_PasswdChangeWarningTimeMinimum)
    {
        LSA_LOG_ERROR("Failed to set PasswdChangeWarningTime to %u.  Minimum is %u.",
                        dwPasswdChangeWarningTime,
                        gProviderLocal_PasswdChangeWarningTimeMinimum
                        );
        dwError = LSA_ERROR_INVALID_PARAMETER;
    }

    if (dwPasswdChangeWarningTime > gProviderLocal_PasswdChangeWarningTimeMaximum)
    {
        LSA_LOG_ERROR("Failed to set PasswdChangeWarningTime to %u.  Maximum is %u.",
                        dwPasswdChangeWarningTime,
                        gProviderLocal_PasswdChangeWarningTimeMaximum
                        );
        dwError = LSA_ERROR_INVALID_PARAMETER;
    }
    BAIL_ON_LSA_ERROR(dwError);
 
    ENTER_LOCAL_GLOBAL_DATA_RW_WRITER_LOCK(bInLock);

    gProviderLocal_PasswdChangeWarningTime = dwPasswdChangeWarningTime;

cleanup:

    LEAVE_LOCAL_GLOBAL_DATA_RW_WRITER_LOCK(bInLock);

    return dwError;

error:

    goto cleanup;
}

DWORD
LsaProviderLocal_GetPasswdChangeWarningTime(
    VOID
    )
{
    DWORD dwPasswdChangeWarningTime = 0;
    BOOLEAN bInLock = FALSE;

    ENTER_LOCAL_GLOBAL_DATA_RW_READER_LOCK(bInLock);
    dwPasswdChangeWarningTime = gProviderLocal_PasswdChangeWarningTime;
    LEAVE_LOCAL_GLOBAL_DATA_RW_READER_LOCK(bInLock);

    return dwPasswdChangeWarningTime;
}
