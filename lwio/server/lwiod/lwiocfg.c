/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

/*
 * Copyright Likewise Software
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

#include "includes.h"

static
DWORD
SMBSrvParseConfig(
    IN PCSTR pszConfigFilePath,
    IN OUT PSMB_CONFIG pConfig
    );

static
DWORD
SMBSrvConfigStartSection(
    PCSTR    pszSectionName,
    PVOID    pData,
    PBOOLEAN pbSkipSection,
    PBOOLEAN pbContinue
    );

static
DWORD
SMBSrvInitializeConfig(
    IN OUT PSMB_CONFIG pConfig
    );

static
DWORD
SMBSrvConfigNameValuePair(
    PCSTR    pszName,
    PCSTR    pszValue,
    PVOID    pData,
    PBOOLEAN pbContinue
    );

static
DWORD
SMBSrvTransferConfigContents(
    PSMB_CONFIG pSrcConfig,
    PSMB_CONFIG pDstConfig
    );

DWORD
SMBSrvSetupInitialConfig(
    VOID
    )
{
    DWORD dwError = 0;
    BOOLEAN bUnlockConfig = FALSE;

    SMB_LOCK_SERVERCONFIG(bUnlockConfig);

    dwError = SMBSrvInitializeConfig(gpServerConfig);

    SMB_UNLOCK_SERVERCONFIG(bUnlockConfig);

    return dwError;
}

DWORD
SMBSrvRefreshConfig(
    PCSTR       pszConfigFilePath
    )
{
    DWORD dwError = 0;
    BOOLEAN bUnlockConfig = FALSE;
    SMB_CONFIG smb_config;

    dwError = SMBSrvParseConfig(
                    pszConfigFilePath,
                    &smb_config);
    BAIL_ON_SMB_ERROR(dwError);

    SMB_LOCK_SERVERCONFIG(bUnlockConfig);

    dwError = SMBSrvTransferConfigContents(
                    &smb_config,
                    gpServerConfig);
    BAIL_ON_SMB_ERROR(dwError);

cleanup:

    SMB_UNLOCK_SERVERCONFIG(bUnlockConfig);

    return dwError;

error:

    SMBSrvFreeConfigContents(&smb_config);

    goto cleanup;
}

static
DWORD
SMBSrvParseConfig(
    IN PCSTR pszConfigFilePath,
    IN OUT PSMB_CONFIG pConfig
    )
{
    DWORD dwError = 0;
    SMB_CONFIG smb_config;

    memset(&smb_config, 0, sizeof(SMB_CONFIG));

    BAIL_ON_INVALID_STRING(pszConfigFilePath);

    dwError = SMBSrvInitializeConfig(&smb_config);
    BAIL_ON_SMB_ERROR(dwError);

    dwError = SMBParseConfigFile(
                    pszConfigFilePath,
                    SMB_CFG_OPTION_STRIP_ALL,
                    &SMBSrvConfigStartSection,
                    NULL,
                    &SMBSrvConfigNameValuePair,
                    NULL,
                    &smb_config);
    BAIL_ON_SMB_ERROR(dwError);

    dwError = SMBSrvTransferConfigContents(
                    &smb_config,
                    pConfig);
    BAIL_ON_SMB_ERROR(dwError);

cleanup:

    return dwError;

error:

    SMBSrvFreeConfigContents(&smb_config);

    goto cleanup;
}

static
DWORD
SMBSrvConfigStartSection(
    PCSTR    pszSectionName,
    PVOID    pData,
    PBOOLEAN pbSkipSection,
    PBOOLEAN pbContinue
    )
{
    DWORD dwError = 0;
    BOOLEAN bContinue = TRUE;
    BOOLEAN bSkipSection = FALSE;

    if (IsNullOrEmptyString(pszSectionName) ||
        (strncasecmp(pszSectionName, "global", sizeof("global")-1)))
    {
        bSkipSection = TRUE;
    }

    *pbSkipSection = bSkipSection;
    *pbContinue = bContinue;

    return dwError;
}

static
DWORD
SMBSrvConfigNameValuePair(
    PCSTR    pszName,
    PCSTR    pszValue,
    PVOID    pData,
    PBOOLEAN pbContinue
    )
{
    DWORD dwError = 0;
    PSMB_CONFIG pConfig = (PSMB_CONFIG)pData;

    BAIL_ON_INVALID_POINTER(pConfig);
    BAIL_ON_INVALID_STRING(pszName);

    *pbContinue = TRUE;

cleanup:

    return dwError;

error:

    *pbContinue = FALSE;

    goto cleanup;
}

static
DWORD
SMBSrvInitializeConfig(
    IN OUT PSMB_CONFIG pConfig
    )
{
    return 0;
}

static
DWORD
SMBSrvTransferConfigContents(
    PSMB_CONFIG pSrcConfig,
    PSMB_CONFIG pDstConfig
    )
{
    SMBSrvFreeConfigContents(pDstConfig);

    memcpy(pDstConfig, pSrcConfig, sizeof(*pSrcConfig));
    memset(pSrcConfig, 0, sizeof(*pSrcConfig));

    return 0;
}

VOID
SMBSrvFreeConfig(
    IN OUT PSMB_CONFIG pConfig
    )
{
    SMBSrvFreeConfigContents(pConfig);

    SMBFreeMemory(pConfig);
}

VOID
SMBSrvFreeConfigContents(
    IN OUT PSMB_CONFIG pConfig
    )
{
    // Nothing to do right now
}

