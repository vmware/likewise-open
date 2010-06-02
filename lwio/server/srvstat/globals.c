/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 */

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
 *        globals.c
 *
 * Abstract:
 *
 *        Likewise IO (LWIO)
 *
 *        Reference Statistics Logging Module (SRV)
 *
 *        Globals
 *
 * Authors: Sriram Nambakam (snambakam@likewise.com)
 *
 */

#include "includes.h"

SRV_STAT_HANDLER_GLOBALS gSrvStatHandlerGlobals =
{
    .config =
        {
            .logTargetType = SRV_STAT_LOG_TARGET_TYPE_SYSLOG,
            .pszPath = NULL
        },
    .fnTable =
        {
            .pfnCreateRequestContext = &LwioSrvStatCreateRequestContext,
            .pfnPushMessage          = &LwioSrvStatPushMessage,
            .pfnSetSubOpCode         = &LwioSrvStatSetSubOpcode,
            .pfnSetIOCTL             = &LwioSrvStatSetIOCTL,
            .pfnSetSessionInfo       = &LwioSrvStatSetSessionInfo,
            .pfnPopMessage           = &LwioSrvStatPopMessage,
            .pfnSetResponseInfo      = &LwioSrvStatSetResponseInfo,
            .pfnCloseRequestContext  = &LwioSrvStatCloseRequestContext
        }
};


