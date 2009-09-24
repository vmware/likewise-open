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
SMBSrvInitializeConfig(
    IN OUT PSMB_CONFIG pConfig
    );

static
DWORD
SMBSrvReadRegistry(
    IN OUT PSMB_CONFIG pConfig
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

    LWIO_LOCK_SERVERCONFIG(bUnlockConfig);

    dwError = SMBSrvInitializeConfig(gpServerConfig);

    LWIO_UNLOCK_SERVERCONFIG(bUnlockConfig);

    return dwError;
}

DWORD
SMBSrvRefreshConfig(
    )
{
    DWORD dwError = 0;
    BOOLEAN bUnlockConfig = FALSE;
    SMB_CONFIG smb_config;

    dwError = SMBSrvReadRegistry(&smb_config);
    BAIL_ON_LWIO_ERROR(dwError);

    LWIO_LOCK_SERVERCONFIG(bUnlockConfig);

    dwError = SMBSrvTransferConfigContents(
                    &smb_config,
                    gpServerConfig);
    BAIL_ON_LWIO_ERROR(dwError);

cleanup:

    LWIO_UNLOCK_SERVERCONFIG(bUnlockConfig);

    return dwError;

error:

    SMBSrvFreeConfigContents(&smb_config);

    goto cleanup;
}

static
DWORD
SMBSrvReadRegistry(
    IN OUT PSMB_CONFIG pConfig
    )
{
    DWORD dwError = 0;
    SMB_CONFIG smb_config;

    memset(&smb_config, 0, sizeof(SMB_CONFIG));

    dwError = SMBSrvInitializeConfig(&smb_config);
    BAIL_ON_LWIO_ERROR(dwError);


    dwError = SMBProcessConfig(
        "Services\\lwio\\Parameters",
        "Policy\\Services\\lwio\\Parameters",
        NULL,
        0);
    BAIL_ON_NON_LWREG_ERROR(dwError);

    dwError = SMBSrvTransferConfigContents(
                    &smb_config,
                    pConfig);
    BAIL_ON_LWIO_ERROR(dwError);

cleanup:

    return dwError;

error:

    SMBSrvFreeConfigContents(&smb_config);

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

