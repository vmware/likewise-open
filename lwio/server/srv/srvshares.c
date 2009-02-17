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
 *        srvshares.c
 *
 * Abstract:
 *
 *        Likewise File System Driver (Srv)
 *
 *       DeviceIo Dispatch Routine
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 */

#include "includes.h"

NTSTATUS
SrvDevCtlAddShare(
    PBYTE lpInBuffer,
    ULONG ulInBufferSize,
    PBYTE lpOutBuffer,
    ULONG ulOutBufferSize
    )
{
    NTSTATUS ntStatus = 0;
    PSHARE_INFO_ADD_PARAMS pAddShareInfoParams = NULL;
    PSHARE_INFO_0 pShareInfo0 = NULL;
    PSHARE_INFO_1 pShareInfo1 = NULL;
    PSHARE_INFO_501 pShareInfo501 = NULL;
    PSHARE_INFO_502 pShareInfo502 = NULL;
    PWSTR pwszShareName = NULL;
    PWSTR pwszPath = NULL;

    ntStatus = LwShareInfoUnmarshalAddParameters(
                        lpInBuffer,
                        ulInBufferSize,
                        &pAddShareInfoParams
                        );
    BAIL_ON_NT_STATUS(ntStatus);

    switch (pAddShareInfoParams->dwInfoLevel)
    {
        case 0:
	    pShareInfo0 = pAddShareInfoParams->info.p0;
	    pwszShareName = pShareInfo0->shi0_netname;
	    pwszPath = NULL;
            break;

        case 1:
	    pShareInfo1 = pAddShareInfoParams->info.p1;
	    pwszShareName = pShareInfo1->shi1_netname;
	    pwszPath = NULL;
            break;

        case 501:
            pShareInfo501 = pAddShareInfoParams->info.p501;
	    pwszShareName = pShareInfo501->shi501_netname;
	    pwszPath = NULL;
            break;

        case 502:
            pShareInfo502 = pAddShareInfoParams->info.p502;
	    pwszShareName = pShareInfo502->shi502_netname;
	    pwszPath = pShareInfo502->shi502_path;

        default:
            ntStatus = STATUS_INVALID_PARAMETER;
            BAIL_ON_NT_STATUS(ntStatus);
            break;

    }

    ntStatus = SrvShareAddShare(
                        pwszShareName,
                        pwszPath
                        );


error:

    return ntStatus;
}
NTSTATUS
SrvDevCtlDeleteShare(
    PBYTE lpInBuffer,
    ULONG ulInBufferSize,
    PBYTE lpOutBuffer,
    ULONG ulOutBufferSize
    )
{
    NTSTATUS ntStatus = 0;

    return ntStatus;
}

NTSTATUS
SrvDevCtlEnumShares(
    PBYTE lpInBuffer,
    ULONG ulInBufferSize,
    PBYTE lpOutBuffer,
    ULONG ulOutBufferSize
    )
{
    NTSTATUS ntStatus = 0;
    ULONG Level = 0;

    switch(Level)
    {
        case 0:
            break;

        case 1:
            break;

        case 2:
            break;

        case 502:
            break;

        case 501:
            break;

        default:

            ntStatus = STATUS_INVALID_PARAMETER;

            break;

    }
    return ntStatus;

}
NTSTATUS
SrvDevCtlGetShareInfo(
    PBYTE lpInBuffer,
    ULONG ulInBufferSize,
    PBYTE lpOutBuffer,
    ULONG ulOutBufferSize
    )
{
    NTSTATUS ntStatus = 0;
    ULONG Level = 0;

    switch (Level)
    {
        case 1:
            break;

        case 2:
            break;

        case 502:
            break;

        case 503:
            break;

        default:

            ntStatus = STATUS_INVALID_PARAMETER;

            break;

    }

    return ntStatus;
}

NTSTATUS
SrvDevCtlSetShareInfo(
    PBYTE lpInBuffer,
    ULONG ulInBufferSize,
    PBYTE lpOutBuffer,
    ULONG ulOutBufferSize
    )
{
    NTSTATUS ntStatus = 0;
    ULONG Level = 0;

    switch(Level)
    {
        case 1:
            break;

        case 2:
            break;

        case 502:
            break;

        case 503:
            break;

        default:

            ntStatus = STATUS_INVALID_PARAMETER;

            break;

    }

    return ntStatus;
}
