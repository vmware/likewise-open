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

NTSTATUS
WireNTTimeToSMBDateTime(
    LONG64    llNTTime,
    PSMB_DATE pSmbDate,
    PSMB_TIME pSmbTime
    )
{
    NTSTATUS ntStatus = 0;
    time_t   timeUnix = 0;
    struct tm stTime = {0};

    timeUnix = (llNTTime /  10000000LL) - 11644473600LL;

    gmtime_r(&timeUnix, &stTime);

    if ((stTime.tm_year + 1900) < 1980)
    {
        /* Minimum time for SMB format is Jan 1, 1980 00:00:00 */
        pSmbDate->usDay      = 1;
        pSmbDate->usMonth    = 1;
        pSmbDate->usYear     = 0;

        pSmbTime->TwoSeconds = 0;
        pSmbTime->Minutes    = 0;
        pSmbTime->Hours      = 0;
    }
    else
    {
        pSmbDate->usDay = stTime.tm_mday;
        pSmbDate->usMonth = stTime.tm_mon + 1;
        pSmbDate->usYear = (stTime.tm_year + 1900) - 1980;

        pSmbTime->TwoSeconds = stTime.tm_sec / 2;
        pSmbTime->Minutes = stTime.tm_min;
        pSmbTime->Hours = stTime.tm_hour;
    }

    return ntStatus;
}


/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
