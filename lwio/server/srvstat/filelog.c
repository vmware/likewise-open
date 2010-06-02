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
 *        filelog.c
 *
 * Abstract:
 *
 *        Likewise IO (LWIO)
 *
 *        Reference Statistics Logging Module (SRV)
 *
 *        Log to File
 *
 * Authors: Sriram Nambakam (snambakam@likewise.com)
 *
 */

#include "includes.h"

NTSTATUS
LwioSrvStatFilelogInit(
    PCSTR                       pszFilePath,
    PSRV_STAT_HANDLER_FILE_LOG* ppFileLog
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PSRV_STAT_HANDLER_FILE_LOG pFileLog = NULL;

    if (!pszFilePath || !*pszFilePath)
    {
        ntStatus = STATUS_INVALID_PARAMETER;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    ntStatus = RTL_ALLOCATE(
                    &pFileLog,
                    SRV_STAT_HANDLER_FILE_LOG,
                    sizeof(SRV_STAT_HANDLER_FILE_LOG));
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = RtlCStringDuplicate(&pFileLog->pszFilePath, pszFilePath);
    BAIL_ON_NT_STATUS(ntStatus);

    if ((pFileLog->fp = fopen(pFileLog->pszFilePath, "w")) == NULL)
    {
        ntStatus = LwErrnoToNtStatus(errno);
        BAIL_ON_NT_STATUS(ntStatus);
    }

    *ppFileLog = pFileLog;

cleanup:

    return ntStatus;

error:

    *ppFileLog = NULL;

    if (pFileLog)
    {
        LwioSrvStatFilelogShutdown(pFileLog);
    }

    goto cleanup;
}

VOID
LwioSrvStatFilelogMessage(
    PSRV_STAT_HANDLER_FILE_LOG pFileLog,
    PCSTR                      pszFormat,
    va_list                    msgList
    )
{
    time_t currentTime;
    struct tm tmp = {0};
    char timeBuf[128];

    currentTime = time(NULL);
    localtime_r(&currentTime, &tmp);

    strftime(timeBuf, sizeof(timeBuf), LWIO_SRV_STAT_LOG_TIME_FORMAT, &tmp);

    fprintf(pFileLog->fp, "%s:info:", timeBuf);
    vfprintf(pFileLog->fp, pszFormat, msgList);
    fprintf(pFileLog->fp, "\n");
    fflush(pFileLog->fp);
}

VOID
LwioSrvStatFilelogShutdown(
    PSRV_STAT_HANDLER_FILE_LOG pFileLog
    )
{
    if (pFileLog->fp)
    {
        fclose(pFileLog->fp);
    }

    RTL_FREE(&pFileLog->pszFilePath);
    RTL_FREE(&pFileLog);
}
