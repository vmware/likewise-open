/*
 * Copyright Likewise Software    2004-2009
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
 *        statisticsapi.h
 *
 * Abstract:
 *
 *        Likewise Input Output (LWIO) - SRV
 *
 *        Statistics API Functions
 *
 * Authors: Sriram Nambakam (snambakam@likewise.com)
 *
 */

#ifndef __SRV_STATISTICS_H__
#define __SRV_STATISTICS_H__

typedef struct _SRV_STAT_INFO * PSRV_STAT_INFO;

NTSTATUS
SrvStatisticsInitialize(
    VOID
    );

BOOLEAN
SrvStatisticsLoggingEnabled(
    VOID
    );

NTSTATUS
SrvStatisticsCreateRequestContext(
    PSRV_STAT_CONNECTION_INFO pConnection,        /* IN              */
    SRV_STAT_SMB_VERSION      protocolVersion,    /* IN              */
    ULONG                     ulRequestLength,    /* IN              */
    PSRV_STAT_INFO*           ppStatInfo          /* IN              */
    );

PSRV_STAT_INFO
SrvStatisticsAcquire(
    PSRV_STAT_INFO pStatInfo
    );

VOID
SrvStatisticsRelease(
    PSRV_STAT_INFO pStatInfo
    );

NTSTATUS
SrvStatisticsSetRequestInfo(
    PSRV_STAT_INFO       pStatInfo,          /* IN              */
    ULONG                ulRequestLength     /* IN              */
    );

NTSTATUS
SrvStatisticsSetResponseCount(
    PSRV_STAT_INFO               pStatInfo,        /* IN              */
    ULONG                        ulNumResponses    /* IN              */
    );

NTSTATUS
SrvStatisticsPushMessage(
    PSRV_STAT_INFO               pStatInfo,        /* IN              */
    ULONG                        ulOpcode,         /* IN              */
    ULONG                        ulMessageLen      /* IN              */
    );

NTSTATUS
SrvStatisticsSetSubOpcode(
    PSRV_STAT_INFO            pStatInfo,           /* IN              */
    ULONG                     ulSubOpcode          /* IN              */
    );

NTSTATUS
SrvStatisticsSetIOCTL(
    PSRV_STAT_INFO            pStatInfo,           /* IN              */
    ULONG                     ulIoCtlCode          /* IN              */
    );

NTSTATUS
SrvStatisticsSetSessionInfo(
    PSRV_STAT_INFO            pStatInfo,           /* IN              */
    PSRV_STAT_SESSION_INFO    pSessionInfo         /* IN              */
    );

NTSTATUS
SrvStatisticsPopMessage(
    PSRV_STAT_INFO            pStatInfo,           /* IN              */
    ULONG                     ulOpCode,            /* IN              */
    ULONG                     ulResponseLength,    /* IN              */
    NTSTATUS                  msgStatus            /* IN              */
    );

NTSTATUS
SrvStatisticsSetResponseInfo(
    PSRV_STAT_INFO            pStatInfo,           /* IN              */
    ULONG                     ulResponseLength     /* IN              */
    );

NTSTATUS
SrvStatisticsCloseRequestContext(
    PSRV_STAT_INFO            pStatInfo            /* IN              */
    );

NTSTATUS
SrvStatisticsShutdown(
    VOID
    );

#endif /* __SRV_STATISTICS_H__ */

