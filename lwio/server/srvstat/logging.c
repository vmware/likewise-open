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
 *        logging.c
 *
 * Abstract:
 *
 *        Likewise IO (LWIO)
 *
 *        Reference Statistics Logging Module (SRV)
 *
 *        Logging
 *
 * Authors: Sriram Nambakam (snambakam@likewise.com)
 *
 */

#include "includes.h"

static
VOID
LwioSrvStatFreeLogger(
    PSRV_STAT_HANDLER_LOGGER pLogger
    );

NTSTATUS
LwioSrvStatLoggingInit(
    VOID
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PSRV_STAT_HANDLER_LOGGER pLogger = NULL;

    ntStatus = RTL_ALLOCATE(
                    &pLogger,
                    SRV_STAT_HANDLER_LOGGER,
                    sizeof(SRV_STAT_HANDLER_LOGGER));
    BAIL_ON_NT_STATUS(ntStatus);

    pLogger->logTargetType = gSrvStatHandlerGlobals.config.logTargetType;

    switch (pLogger->logTargetType)
    {
        case SRV_STAT_LOG_TARGET_TYPE_SYSLOG:

            ntStatus = LwioSrvStatSyslogInit(&pLogger->pSysLog);

            break;

        case SRV_STAT_LOG_TARGET_TYPE_FILE:

            ntStatus = LwioSrvStatFilelogInit(
                            gSrvStatHandlerGlobals.config.pszPath,
                            &pLogger->pFileLog);

            break;

        default:

            ntStatus = STATUS_INVALID_PARAMETER;

            break;
    }

    gSrvStatHandlerGlobals.pLogger = pLogger;

cleanup:

    return ntStatus;

error:

    if (pLogger)
    {
        LwioSrvStatFreeLogger(pLogger);
    }

    goto cleanup;
}

VOID
LwioSrvStatLogMessage(
    PSRV_STAT_HANDLER_LOGGER pLogger,
    PCSTR                    pszFormat,
    va_list                  msgList
    )
{
    switch (pLogger->logTargetType)
    {
        case SRV_STAT_LOG_TARGET_TYPE_SYSLOG:

            LwioSrvStatSyslogMessage(pLogger->pSysLog, pszFormat, msgList);

            break;

        case SRV_STAT_LOG_TARGET_TYPE_FILE:

            LwioSrvStatFilelogMessage(pLogger->pFileLog, pszFormat, msgList);

            break;

        default:

            break;
    }
}

VOID
LwioSrvStatLoggingShutdown(
    VOID
    )
{
    BOOLEAN bInLock = FALSE;

    SRV_STAT_HANDLER_LOCK_MUTEX(bInLock, &gSrvStatHandlerGlobals.mutex);

    if (gSrvStatHandlerGlobals.pLogger)
    {
        LwioSrvStatFreeLogger(gSrvStatHandlerGlobals.pLogger);
        gSrvStatHandlerGlobals.pLogger = NULL;
    }

    SRV_STAT_HANDLER_UNLOCK_MUTEX(bInLock, &gSrvStatHandlerGlobals.mutex);
}

static
VOID
LwioSrvStatFreeLogger(
    PSRV_STAT_HANDLER_LOGGER pLogger
    )
{
    switch (pLogger->logTargetType)
    {
        case SRV_STAT_LOG_TARGET_TYPE_SYSLOG:

            if (pLogger->pSysLog)
            {
                LwioSrvStatSyslogShutdown(pLogger->pSysLog);
            }

            break;

        case SRV_STAT_LOG_TARGET_TYPE_FILE:

            if (pLogger->pFileLog)
            {
                LwioSrvStatFilelogShutdown(pLogger->pFileLog);
            }

            break;

        default:

            break;
    }

    RTL_FREE(&pLogger);
}
