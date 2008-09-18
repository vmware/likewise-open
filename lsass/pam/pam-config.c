/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

/*
 * Copyright Likewise Software    2004-2008
 * All rights reserved.
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
 *        pam-config.c
 *
 * Abstract:
 * 
 *        Likewise Security and Authentication Subsystem (LSASS)
 * 
 *        Pluggable Authentication Module
 * 
 *        Configuration API
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 */
#include "pam-lsass.h"

#define LSA_PAM_CONFIG_FILE_PATH CONFIGDIR "/lsassd.conf"

VOID
LsaPamReadConfigFile(
    VOID
    )
{
    DWORD dwError = 0;
    BOOLEAN bExists = FALSE;
    DWORD dwSeenPamSection = FALSE;
    
    dwError = LsaCheckFileExists(
                LSA_PAM_CONFIG_FILE_PATH,
                &bExists);
    BAIL_ON_LSA_ERROR(dwError);
    
    if (!bExists) {
        goto cleanup;
    }
    
    dwError = LsaParseConfigFile(
                LSA_PAM_CONFIG_FILE_PATH,
                LSA_CFG_OPTION_STRIP_ALL,
                &LsaPamConfigStartSection,
                NULL,
                &LsaPamConfigNameValuePair,
                NULL,
                (PVOID)&dwSeenPamSection);
    BAIL_ON_LSA_ERROR(dwError);
    
cleanup:

    return;
    
error:

    goto cleanup;
}

DWORD
LsaPamConfigStartSection(
    PCSTR    pszSectionName,
    PVOID    pData,
    PBOOLEAN pbSkipSection,
    PBOOLEAN pbContinue
    )
{
    DWORD   dwError = 0;
    BOOLEAN bSkipSection = FALSE;
    BOOLEAN bContinue = TRUE;
    PDWORD  pdwSeenPamSection = (PDWORD)pData;
    
    if (*pdwSeenPamSection == TRUE) {
        bContinue = FALSE;
        bSkipSection = TRUE;
        goto done;
    }
    
    if (!IsNullOrEmptyString(pszSectionName) &&
        !strcasecmp(pszSectionName, "pam")) {
        *pdwSeenPamSection = TRUE;
    } else {
        bSkipSection = TRUE;
    }
    
done:

    *pbSkipSection = bSkipSection;
    *pbContinue = bContinue;

    return dwError;
}

DWORD
LsaPamConfigNameValuePair(
    PCSTR    pszName,
    PCSTR    pszValue,
    PVOID    pData,
    PBOOLEAN pbContinue
    )
{
    DWORD dwError = 0;
    
    if (IsNullOrEmptyString(pszName) ||
        IsNullOrEmptyString(pszValue)) {
        goto cleanup;
    }
    
    if (!strcasecmp(pszName, "log-level")) {
        
        if (!strcasecmp(pszValue, "error")) {
            gdwLogLevel = PAM_LOG_LEVEL_ERROR;
        } else if (!strcasecmp(pszValue, "warning")) {
            gdwLogLevel = PAM_LOG_LEVEL_WARNING;
        } else if (!strcasecmp(pszValue, "info")) {
            gdwLogLevel = PAM_LOG_LEVEL_INFO;
        } else if (!strcasecmp(pszValue, "verbose")) {
            gdwLogLevel = PAM_LOG_LEVEL_VERBOSE;
        } else if (!strcasecmp(pszValue, "debug")) {
            gdwLogLevel = PAM_LOG_LEVEL_VERBOSE;
            gdwLogDebug = TRUE;
        }
    }
    else if (!strcasecmp(pszName, "display-motd")) {
        
        if (!strcasecmp(pszValue, "yes")) {
            gbLsaPamDisplayMOTD = TRUE;
        } else {
            gbLsaPamDisplayMOTD = FALSE;
        } 
    }
    
cleanup:

    return dwError;
}

