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
 *        config.c
 *
 * Abstract:
 *
 *        Likewise I/O (LWIO) - SRV
 *
 *        Configuration
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 */

#include "includes.h"

static
NTSTATUS
SrvTransferConfigContents(
    PLWIO_SRV_CONFIG pSrc,
    PLWIO_SRV_CONFIG pDest
    );


BOOLEAN gbSupportSMB2;

static
SMB_CONFIG_TABLE gConfig[] = {
    {
        "SupportSmb2",
        FALSE,
        SMBTypeBoolean,
        0,
        -1,
        NULL,
        &gbSupportSMB2
    }
};

NTSTATUS
SrvReadConfig(
    PLWIO_SRV_CONFIG pConfig
    )
{
    NTSTATUS        ntStatus = STATUS_SUCCESS;
    ULONG           ulError = 0;
    LWIO_SRV_CONFIG srvConfig;

    ntStatus = SrvInitConfig(&srvConfig);
    BAIL_ON_NT_STATUS(ntStatus);

    gbSupportSMB2             = FALSE;

    ulError = SMBProcessConfig(
                "Services\\lwio\\Parameters\\Drivers",
                "Policy\\Services\\lwio\\Parameters\\Drivers",
                gConfig,
                sizeof(gConfig)/sizeof(gConfig[0]));

    srvConfig.bSupportSMB2 = gbSupportSMB2;

    if (ulError)
    {
        LWIO_LOG_ERROR("Failed to parse device configuration [error code: %u]",
                       ulError);

        ntStatus = STATUS_DEVICE_CONFIGURATION_ERROR;
    }
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SrvTransferConfigContents(&srvConfig, pConfig);
    BAIL_ON_NT_STATUS(ntStatus);

cleanup:

    SrvFreeConfigContents(&srvConfig);

    return ntStatus;

error:

    goto cleanup;
}

NTSTATUS
SrvInitConfig(
    PLWIO_SRV_CONFIG pConfig
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;

    SrvFreeConfigContents(pConfig);

    pConfig->ulMaxNumPackets          = LWIO_SRV_DEFAULT_NUM_MAX_PACKETS;
    pConfig->ulNumWorkers             = LWIO_SRV_DEFAULT_NUM_WORKERS;
    pConfig->ulMaxNumWorkItemsInQueue = LWIO_SRV_DEFAULT_NUM_MAX_QUEUE_ITEMS;
    pConfig->bSupportSMB2             = FALSE;

    return ntStatus;
}

static
NTSTATUS
SrvTransferConfigContents(
    PLWIO_SRV_CONFIG pSrc,
    PLWIO_SRV_CONFIG pDest
    )
{
    SrvFreeConfigContents(pDest);

    *pDest = *pSrc;

    SrvFreeConfigContents(pSrc);

    return 0;
}

VOID
SrvFreeConfigContents(
    PLWIO_SRV_CONFIG pConfig
    )
{
    // Nothing to free right now
    memset(pConfig, 0, sizeof(*pConfig));
}
