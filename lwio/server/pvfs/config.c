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
 *        Likewise Posix File System Driver (PVFS)
 *
 *        Configuration settings
 *
 * Authors: Gerald Carter <gcarter@likewise.com>
 */

#include "pvfs.h"


/***********************************************************************
 **********************************************************************/

static
NTSTATUS
PvfsConfigDefaultInit(
    PPVFS_DRIVER_CONFIG pConfig
    );


NTSTATUS
PvfsConfigRegistryInit(
    PPVFS_DRIVER_CONFIG pConfig
    )
{
    NTSTATUS ntError = STATUS_SUCCESS;
    PLWIO_CONFIG_REG pReg = NULL;
    DWORD ZctMode = 0;

    ntError = PvfsConfigDefaultInit(pConfig);
    BAIL_ON_NT_STATUS(ntError);

    ntError = LwIoOpenConfig(
                   PVFS_CONF_REGISTRY_LOCAL,
                   PVFS_CONF_REGISTRY_POLICY,
                   &pReg);
    if (ntError)
    {
        LWIO_LOG_ERROR(
            "%s: Failed to access device configuration [error code: %x]",
            PVFS_LOG_HEADER,
            ntError);

        ntError = STATUS_DEVICE_CONFIGURATION_ERROR;
    }
    BAIL_ON_NT_STATUS(ntError);

    /* Ignore error as it may not exist; we can still use default. */

    LwIoReadConfigBoolean(
        pReg,
        "EnableOplocks",
        TRUE,
        &pConfig->EnableOplocks);

    LwIoReadConfigBoolean(
        pReg,
        "EnableFullAsync",
        TRUE,
        &pConfig->EnableFullAsync);

    LwIoReadConfigBoolean(
        pReg,
        "EnableDriverDebug",
        TRUE,
        &pConfig->EnableDriverDebug);

    LwIoReadConfigDword(
        pReg,
        "ZctMode",
        TRUE,
        0,
        0xffffffff,
        &ZctMode);

    pConfig->ZctMode = (PVFS_ZCT_MODE) ZctMode;

    LwIoReadConfigDword(
        pReg,
        "WorkerThreadPoolSize",
        TRUE,
        1,
        512,
        &pConfig->WorkerThreadPoolSize);

    LwIoReadConfigDword(
        pReg,
        "CreateFileMode",
        TRUE,
        1,
        0x00000fff,
        &pConfig->CreateFileMode);

    LwIoReadConfigDword(
        pReg,
        "CreateDirectoryMode",
        TRUE,
        1,
        0x00000fff,
        &pConfig->CreateDirectoryMode);

    LwIoReadConfigDword(
        pReg,
        "VirtualOwner",
        TRUE,
        0,
        0xffffffff,
        (PDWORD)&pConfig->VirtualUid);

    LwIoReadConfigDword(
        pReg,
        "VirtualGroup",
        TRUE,
        0,
        0xffffffff,
        (PDWORD)&pConfig->VirtualGid);


cleanup:

    if (pReg)
    {
        LwIoCloseConfig(pReg);
    }

    return ntError;

error:

    goto cleanup;
}


/***********************************************************************
 **********************************************************************/

static
NTSTATUS
PvfsConfigDefaultInit(
    PPVFS_DRIVER_CONFIG pConfig
    )
{
    pthread_rwlock_init(&pConfig->rwLock, NULL);

    pConfig->CreateFileMode = 0700;
    pConfig->CreateDirectoryMode = 0700;

    pConfig->EnableOplocks = TRUE;
    pConfig->EnableFullAsync = FALSE;
    pConfig->EnableDriverDebug = FALSE;
    pConfig->ZctMode = PVFS_ZCT_MODE_DISABLED;

    pConfig->VirtualUid = (uid_t)-1;
    pConfig->VirtualGid = (gid_t)-1;

    pConfig->WorkerThreadPoolSize = 4;

    return STATUS_SUCCESS;
}


/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
