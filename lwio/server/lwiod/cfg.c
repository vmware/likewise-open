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

static
NTSTATUS
LwioSrvInitializeConfig(
    IN OUT PLWIO_CONFIG pConfig
    );

static
NTSTATUS
LwioSrvReadRegistry(
    IN OUT PLWIO_CONFIG pConfig
    );

static
NTSTATUS
LwioSrvTransferConfigContents(
    PLWIO_CONFIG pSrcConfig,
    PLWIO_CONFIG pDstConfig
    );

NTSTATUS
LwioSrvSetupInitialConfig(
    VOID
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    BOOLEAN bUnlockConfig = FALSE;

    LWIO_LOCK_SERVERCONFIG(bUnlockConfig);

    ntStatus = LwioSrvInitializeConfig(gpServerConfig);

    LWIO_UNLOCK_SERVERCONFIG(bUnlockConfig);

    return ntStatus;
}

NTSTATUS
LwioSrvRefreshConfig(
    VOID
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    BOOLEAN bUnlockConfig = FALSE;
    LWIO_CONFIG LwIoConfig;

    ntStatus = LwioSrvReadRegistry(&LwIoConfig);
    BAIL_ON_NT_STATUS(ntStatus);

    LWIO_LOCK_SERVERCONFIG(bUnlockConfig);

    ntStatus = LwioSrvTransferConfigContents(
                    &LwIoConfig,
                    gpServerConfig);
    BAIL_ON_NT_STATUS(ntStatus);

cleanup:

    LWIO_UNLOCK_SERVERCONFIG(bUnlockConfig);

    return ntStatus;

error:

    LwioSrvFreeConfigContents(&LwIoConfig);

    goto cleanup;
}

static
NTSTATUS
LwioSrvReadRegistry(
    IN OUT PLWIO_CONFIG pConfig
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    LWIO_CONFIG LwIoConfig;

    memset(&LwIoConfig, 0, sizeof(LWIO_CONFIG));

    ntStatus = LwioSrvInitializeConfig(&LwIoConfig);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = LwIoProcessConfig(
        "Services\\lwio\\Parameters",
        "Policy\\Services\\lwio\\Parameters",
        NULL,
        0,
        TRUE);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = LwioSrvTransferConfigContents(
                    &LwIoConfig,
                    pConfig);
    BAIL_ON_NT_STATUS(ntStatus);

cleanup:

    return ntStatus;

error:

    LwioSrvFreeConfigContents(&LwIoConfig);

    goto cleanup;
}

static
NTSTATUS
LwioSrvInitializeConfig(
    IN OUT PLWIO_CONFIG pConfig
    )
{
    return STATUS_SUCCESS;
}

static
NTSTATUS
LwioSrvTransferConfigContents(
    PLWIO_CONFIG pSrcConfig,
    PLWIO_CONFIG pDstConfig
    )
{
    LwioSrvFreeConfigContents(pDstConfig);

    memcpy(pDstConfig, pSrcConfig, sizeof(*pSrcConfig));
    memset(pSrcConfig, 0, sizeof(*pSrcConfig));

    return STATUS_SUCCESS;
}

VOID
LwioSrvFreeConfig(
    IN OUT PLWIO_CONFIG pConfig
    )
{
    LwioSrvFreeConfigContents(pConfig);

    LwIoFreeMemory(pConfig);
}

VOID
LwioSrvFreeConfigContents(
    IN OUT PLWIO_CONFIG pConfig
    )
{
    // Nothing to do right now
}


/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
