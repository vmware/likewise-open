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
#include "includes.h"

#define LSA_PAM_CONFIG_FILE_PATH CONFIGDIR "/lsassd.conf"
#define LSA_PAM_LOGON_RIGHTS_DENIED_MESSAGE "Access denied"

typedef struct _LSA_CONFIG_READER_CONTEXT
{
    DWORD           dwSeenPamSection;
    PLSA_PAM_CONFIG pConfig;
} LSA_CONFIG_READER_CONTEXT, *PLSA_CONFIG_READER_CONTEXT;

typedef DWORD (*PFN_PAM_CONFIG_HANDLER)(
                    PLSA_PAM_CONFIG pConfig,
                    PCSTR          pszName,
                    PCSTR          pszValue
                    );

typedef struct __PAM_CONFIG_HANDLER
{
    PCSTR                  pszId;
    PFN_PAM_CONFIG_HANDLER pfnHandler;
} PAM_CONFIG_HANDLER, *PPAM_CONFIG_HANDLER;

static
DWORD
LsaPam_SetConfig_LogLevel(
    PLSA_PAM_CONFIG pConfig,
    PCSTR           pszName,
    PCSTR           pszValue
    );

static
DWORD
LsaPam_SetConfig_DisplayMOTD(
    PLSA_PAM_CONFIG pConfig,
    PCSTR           pszName,
    PCSTR           pszValue
    );

static
DWORD
LsaPam_SetConfig_UserNotAllowedError(
    PLSA_PAM_CONFIG pConfig,
    PCSTR           pszName,
    PCSTR           pszValue
    );

static
DWORD
LsaPamConfigStartSection(
    PCSTR    pszSectionName,
    PVOID    pData,
    PBOOLEAN pbSkipSection,
    PBOOLEAN pbContinue
    );

static
DWORD
LsaPamConfigNameValuePair(
    PCSTR    pszName,
    PCSTR    pszValue,
    PVOID    pData,
    PBOOLEAN pbContinue
    );

static PAM_CONFIG_HANDLER gConfigHandlers[] =
{
    { "log-level",              &LsaPam_SetConfig_LogLevel },
    { "display-motd",           &LsaPam_SetConfig_DisplayMOTD },
    { "user-not-allowed-error", &LsaPam_SetConfig_UserNotAllowedError }
};

DWORD
LsaPamReadConfigFile(
    PLSA_PAM_CONFIG* ppConfig
    )
{
    DWORD dwError = 0;
    BOOLEAN bExists = FALSE;
    PLSA_PAM_CONFIG pConfig = NULL;
    LSA_CONFIG_READER_CONTEXT context = {0};

    dwError = LwAllocateMemory(
                    sizeof(LSA_PAM_CONFIG),
                    (PVOID*)&pConfig);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaPamInitializeConfig(pConfig);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaCheckFileExists(
                LSA_PAM_CONFIG_FILE_PATH,
                &bExists);
    BAIL_ON_LSA_ERROR(dwError);

    if (!bExists) {
        goto done;
    }

    context.pConfig = pConfig;

    dwError = LsaParseConfigFile(
                LSA_PAM_CONFIG_FILE_PATH,
                LSA_CFG_OPTION_STRIP_ALL,
                &LsaPamConfigStartSection,
                NULL,
                &LsaPamConfigNameValuePair,
                NULL,
                (PVOID)&context);
    BAIL_ON_LSA_ERROR(dwError);

done:

    *ppConfig = pConfig;

cleanup:

    return dwError;

error:

    if (pConfig)
    {
        LsaPamFreeConfig(pConfig);
    }

    goto cleanup;
}

DWORD
LsaPamInitializeConfig(
    PLSA_PAM_CONFIG pConfig
    )
{
    DWORD dwError = 0;
    PSTR  pszMessage = NULL;

    memset(pConfig, 0, sizeof(LSA_PAM_CONFIG));

    pConfig->bLsaPamDisplayMOTD = FALSE;
    pConfig->dwLogLevel = PAM_LOG_LEVEL_ERROR;

    dwError = LwAllocateString(
                    LSA_PAM_LOGON_RIGHTS_DENIED_MESSAGE,
                    &pszMessage);
    BAIL_ON_LSA_ERROR(dwError);

    LW_SAFE_FREE_STRING(pConfig->pszAccessDeniedMessage);
    pConfig->pszAccessDeniedMessage = pszMessage;

error:

    return dwError;
}

VOID
LsaPamFreeConfig(
    PLSA_PAM_CONFIG pConfig
    )
{
    LsaPamFreeConfigContents(pConfig);
    LwFreeMemory(pConfig);
}

VOID
LsaPamFreeConfigContents(
    PLSA_PAM_CONFIG pConfig
    )
{
    LW_SAFE_FREE_STRING(pConfig->pszAccessDeniedMessage);
}

static
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
    PLSA_CONFIG_READER_CONTEXT  pContext = (PLSA_CONFIG_READER_CONTEXT)pData;

    if (pContext->dwSeenPamSection == TRUE) {
        bContinue = FALSE;
        bSkipSection = TRUE;
        goto done;
    }

    if (!LW_IS_NULL_OR_EMPTY_STR(pszSectionName) &&
        !strcasecmp(pszSectionName, "pam")) {
        pContext->dwSeenPamSection = TRUE;
    } else {
        bSkipSection = TRUE;
    }

done:

    *pbSkipSection = bSkipSection;
    *pbContinue = bContinue;

    return dwError;
}

static
DWORD
LsaPamConfigNameValuePair(
    PCSTR    pszName,
    PCSTR    pszValue,
    PVOID    pData,
    PBOOLEAN pbContinue
    )
{
    DWORD dwError = 0;

    if (!LW_IS_NULL_OR_EMPTY_STR(pszName))
    {
        DWORD iHandler = 0;
        DWORD nHandlers = sizeof(gConfigHandlers)/sizeof(gConfigHandlers[0]);

        for (; iHandler < nHandlers; iHandler++)
        {
            if (!strcasecmp(gConfigHandlers[iHandler].pszId, pszName))
            {
                PLSA_CONFIG_READER_CONTEXT  pContext = (PLSA_CONFIG_READER_CONTEXT)pData;
                PLSA_PAM_CONFIG pConfig = pContext->pConfig;

                gConfigHandlers[iHandler].pfnHandler(
                                pConfig,
                                pszName,
                                pszValue);
                break;
            }
        }
    }

    *pbContinue = TRUE;

    return dwError;
}

static
DWORD
LsaPam_SetConfig_LogLevel(
    PLSA_PAM_CONFIG pConfig,
    PCSTR           pszName,
    PCSTR           pszValue
    )
{
    if (LW_IS_NULL_OR_EMPTY_STR(pszValue))
    {
        pConfig->dwLogLevel = PAM_LOG_LEVEL_DISABLED;
    }
    else if (!strcasecmp(pszValue, "error")) {
        pConfig->dwLogLevel = PAM_LOG_LEVEL_ERROR;
    } else if (!strcasecmp(pszValue, "warning")) {
        pConfig->dwLogLevel = PAM_LOG_LEVEL_WARNING;
    } else if (!strcasecmp(pszValue, "info")) {
        pConfig->dwLogLevel = PAM_LOG_LEVEL_INFO;
    } else if (!strcasecmp(pszValue, "verbose")) {
        pConfig->dwLogLevel = PAM_LOG_LEVEL_VERBOSE;
    } else if (!strcasecmp(pszValue, "debug")) {
        pConfig->dwLogLevel = PAM_LOG_LEVEL_DEBUG;
    }
    else
    {
        pConfig->dwLogLevel = PAM_LOG_LEVEL_DISABLED;
    }

    return 0;
}

static
DWORD
LsaPam_SetConfig_DisplayMOTD(
    PLSA_PAM_CONFIG pConfig,
    PCSTR           pszName,
    PCSTR           pszValue
    )
{
    if (!LW_IS_NULL_OR_EMPTY_STR(pszValue) &&
        (!strcasecmp(pszValue, "true") ||
         (*pszValue == 'Y') ||
         (*pszValue == 'y')))
    {
        pConfig->bLsaPamDisplayMOTD = TRUE;
    }
    else
    {
        pConfig->bLsaPamDisplayMOTD = FALSE;
    }

    return 0;
}

static
DWORD
LsaPam_SetConfig_UserNotAllowedError(
    PLSA_PAM_CONFIG pConfig,
    PCSTR           pszName,
    PCSTR           pszValue
    )
{
    DWORD dwError = 0;
    PSTR  pszMessage = NULL;

    if (!LW_IS_NULL_OR_EMPTY_STR(pszValue))
    {
        dwError = LwAllocateString(
                        pszValue,
                        &pszMessage);
        BAIL_ON_LSA_ERROR(dwError);

        LW_SAFE_FREE_STRING(pConfig->pszAccessDeniedMessage);
        pConfig->pszAccessDeniedMessage = pszMessage;
    }

cleanup:

    return dwError;

error:

    LW_SAFE_FREE_STRING(pszMessage);

    goto cleanup;
}
