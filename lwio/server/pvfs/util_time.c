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
 *        file_basic_info.c
 *
 * Abstract:
 *
 *        Likewise Posix File System Driver (PVFS)
 *
 *        FileBasicInformation Handler
 *
 * Authors: Gerald Carter <gcarter@likewise.com>
 */

#include "pvfs.h"

#define TIME_SEC_CONVERSION_CONSTANT     11644473600LL

/**
 * Converts a time_t value (seconds since Jan1, 1970)
 * to a Windows time value (100's of nanoseconds since
 * Jan 1, 1601).
 *     NTTime = A * (UnixTime + B)
 *     A = 10^7 (100 nanosecond ticks)
 *     B = 11644473600 (sec between 1/1/1601 and 1/1/1970)
 */

NTSTATUS
PvfsUnixToWinTime(
    PLONG64 pWinTime,
    time_t UnixTime
	)
{
    NTSTATUS ntError = STATUS_UNSUCCESSFUL;

    BAIL_ON_INVALID_PTR(pWinTime, ntError);

    *pWinTime = (UnixTime + TIME_SEC_CONVERSION_CONSTANT) * 10000000LL;

    ntError = STATUS_SUCCESS;

cleanup:
    return ntError;

error:
    goto cleanup;

}


/**
 * Converts a Windows time value to a Unix time_t
 * Refer to PvfsUnixToWinTime() for details
 */

NTSTATUS
PvfsWinToUnixTime(
    time_t *pUnixTime,
    LONG64 WinTime
	)
{
    NTSTATUS ntError = STATUS_UNSUCCESSFUL;

    BAIL_ON_INVALID_PTR(pUnixTime, ntError);

    *pUnixTime = (WinTime /  10000000LL) - TIME_SEC_CONVERSION_CONSTANT;

    ntError=  STATUS_SUCCESS;

cleanup:
    return ntError;

error:
    goto cleanup;
}


/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/

