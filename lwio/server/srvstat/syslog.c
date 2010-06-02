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
 *        syslog.c
 *
 * Abstract:
 *
 *        Likewise IO (LWIO)
 *
 *        Reference Statistics Logging Module (SRV)
 *
 *        Log to syslog
 *
 * Authors: Sriram Nambakam (snambakam@likewise.com)
 *
 */

#include "includes.h"

NTSTATUS
LwioSrvStatSyslogInit(
    PSRV_STAT_HANDLER_SYS_LOG* ppSysLog
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PSRV_STAT_HANDLER_SYS_LOG pSysLog = NULL;
    ULONG ulOptions  = LOG_PID;
    ULONG ulFacility = LOG_DAEMON;

    ntStatus = RTL_ALLOCATE(
                    &pSysLog,
                    SRV_STAT_HANDLER_SYS_LOG,
                    sizeof(SRV_STAT_HANDLER_SYS_LOG));
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = RtlCStringDuplicate(&pSysLog->pszIdentifier, "lwiosrvstat");
    BAIL_ON_NT_STATUS(ntStatus);

    pSysLog->ulFacility = ulFacility;
    pSysLog->ulOptions = ulOptions;

    openlog(pSysLog->pszIdentifier, pSysLog->ulOptions, pSysLog->ulFacility);

    setlogmask(LOG_INFO);

    pSysLog->bOpened = TRUE;

    *ppSysLog = pSysLog;

cleanup:

    return ntStatus;

error:

    *ppSysLog = NULL;

    if (pSysLog)
    {
        LwioSrvStatSyslogShutdown(pSysLog);
    }

    goto cleanup;
}

VOID
LwioSrvStatSyslogMessage(
    PSRV_STAT_HANDLER_SYS_LOG pSysLog,
    PCSTR                     pszFormat,
    va_list                   msgList
    )
{
    #if defined(HAVE_VSYSLOG)
        vsyslog(LOG_INFO, pszFormat, msgList);
    #else
        NTSTATUS ntStatus;
        PSTR pszBuffer = NULL;

        ntStatus = RtlCStringAllocatePrintf(&pszBuffer, pszFormat, msgList);
        if (ntStatus == STATUS_SUCCESS)
        {
            syslog(LOG_INFO, "%s", pszBuffer);
        }

        RTL_FREE(&pszBuffer);
    #endif /* ! HAVE_VSYSLOG */
}

VOID
LwioSrvStatSyslogShutdown(
    PSRV_STAT_HANDLER_SYS_LOG pSysLog
    )
{
    if (pSysLog->bOpened)
    {
        closelog();
    }

    RTL_FREE(&pSysLog->pszIdentifier);
    RTL_FREE(&pSysLog);
}
