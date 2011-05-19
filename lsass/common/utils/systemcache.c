/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

/*
 * Copyright Likewise Software    2011
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
 *        systemcache.c
 *
 * Abstract:
 *
 *        Likewise Security and Authentication Subsystem (LSASS)
 *
 *        A function to flush the operating system's user/group cache.
 *
 * Authors:
 *          Kyle Stemen (kstemen@likewisesoftware.com)
 */

#include "includes.h"

#if defined (__LWI_DARWIN__)
#include <mach/mach_init.h>
#include <mach/message.h>
#include <servers/bootstrap.h>
#include <DirectoryService/DirServicesConst.h>

#include "DSlibinfoMIG.h"
#endif /* defined (__LWI_DARWIN__) */

DWORD
LsaUtilFlushSystemCache(
    VOID
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;

#if defined (__LWI_DARWIN__)
    char reply[16384] = { 0, };
    mach_msg_type_number_t replyCnt = 0;
    vm_offset_t ooreply = 0;
    mach_msg_type_number_t ooreplyCnt = 0;
    int32_t procno = 0;
    security_token_t userToken;
    mach_port_t serverPort;

    if (bootstrap_look_up(
                bootstrap_port,
                kDSStdMachDSLookupPortName,
                &serverPort) != KERN_SUCCESS)
    {
        BAIL_WITH_LSA_ERROR(LW_ERROR_INTERNAL);
    }

    if (libinfoDSmig_GetProcedureNumber(
                    serverPort,
                    "_flushcache",
                    &procno,
                    &userToken) != KERN_SUCCESS)
    {
        BAIL_WITH_LSA_ERROR(LW_ERROR_INTERNAL);
    }

    if (libinfoDSmig_Query(
            serverPort,
            procno,
            "",
            0,
            reply,
            &replyCnt,
            &ooreply,
            &ooreplyCnt,
            &userToken) != KERN_SUCCESS)
    {
        BAIL_WITH_LSA_ERROR(LW_ERROR_INTERNAL);
    }

#endif /* defined (__LWI_DARWIN__) */

cleanup:
    return dwError;

#if defined (__LWI_DARWIN__)
error:
#endif /* defined (__LWI_DARWIN__) */
    goto cleanup;
}
