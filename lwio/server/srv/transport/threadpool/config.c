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
 *        Likewise I/O Subsystem
 *
 *        SRV Threadpool Transport
 *
 *        Configuration
 *
 * Authors: Sriram Nambakam (snambakam@likewise.com)
 *
 */

#include "includes.h"

static
NTSTATUS
SrvThreadpoolTransportTransferConfigContents(
    PLWIO_SRV_THREADPOOL_TRANSPORT_CONFIG pSrcConfig,
    PLWIO_SRV_THREADPOOL_TRANSPORT_CONFIG pDstConfig
    );

NTSTATUS
SrvThreadpoolTransportInitConfig(
    PLWIO_SRV_THREADPOOL_TRANSPORT_CONFIG pConfig
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;

    pConfig->bEnableSigning   = FALSE;
    pConfig->bRequireSigning = FALSE;

    return ntStatus;
}

NTSTATUS
SrvThreadpoolTransportReadConfig(
    PLWIO_SRV_THREADPOOL_TRANSPORT_CONFIG pConfig
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    LWIO_SRV_THREADPOOL_TRANSPORT_CONFIG config = {0};
    PLWIO_CONFIG_REG pReg = NULL;
    DWORD           dwError = 0;

    ntStatus = SrvThreadpoolTransportInitConfig(&config);
    BAIL_ON_NT_STATUS(ntStatus);

    dwError = LwIoOpenConfig(
                    "Services\\lwio\\Parameters\\Drivers\\srv",
                    "Policy\\Services\\lwio\\Parameters\\Drivers\\srv",
                    &pReg);
    if (dwError)
    {
        LWIO_LOG_ERROR("Failed to access device configuration [error code: %u]",
                       dwError);

        ntStatus = STATUS_DEVICE_CONFIGURATION_ERROR;
    }
    BAIL_ON_NT_STATUS(ntStatus);

    /* Ignore errors */
    LwIoReadConfigBoolean(
                    pReg,
                    "EnableSecuritySignatures",
                    TRUE,
                    &(config.bEnableSigning));

    LwIoReadConfigBoolean(
                    pReg,
                    "RequireSecuritySignatures",
                    TRUE,
                    &(config.bRequireSigning));

    ntStatus = SrvThreadpoolTransportTransferConfigContents(&config, pConfig);
    BAIL_ON_NT_STATUS(ntStatus);

cleanup:

    if (pReg)
    {
        LwIoCloseConfig(pReg);
    }

    SrvThreadpoolTransportFreeConfigContents(&config);

    return ntStatus;

error:

    goto cleanup;
}

static
NTSTATUS
SrvThreadpoolTransportTransferConfigContents(
    PLWIO_SRV_THREADPOOL_TRANSPORT_CONFIG pSrcConfig,
    PLWIO_SRV_THREADPOOL_TRANSPORT_CONFIG pDstConfig
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;

    memset(pDstConfig, 0, sizeof(LWIO_SRV_THREADPOOL_TRANSPORT_CONFIG));
    memcpy(pDstConfig, pSrcConfig, sizeof(LWIO_SRV_THREADPOOL_TRANSPORT_CONFIG));
    memset(pSrcConfig, 0, sizeof(LWIO_SRV_THREADPOOL_TRANSPORT_CONFIG));

    return ntStatus;
}

VOID
SrvThreadpoolTransportFreeConfigContents(
    PLWIO_SRV_THREADPOOL_TRANSPORT_CONFIG pConfig
    )
{
}
