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
 *        prototypes.h
 *
 * Abstract:
 *
 *        Likewise IO (LWIO) - SRV
 *
 *        Elements API
 *
 *        prototypes
 *
 * Authors: Sriram Nambakam (snambakam@likewise.com)
 *
 */

// config.c

NTSTATUS
SrvElementsConfigSetupInitial(
    VOID
    );

NTSTATUS
SrvElementsConfigRefresh(
    VOID
    );

ULONG
SrvElementsConfigGetGlobalCreditLimit(
    VOID
    );

USHORT
SrvElementsConfigGetClientCreditLimit(
    VOID
    );

VOID
SrvElementsConfigShutdown(
    VOID
    );

// creditor.c

NTSTATUS
SrvCreditorCreate(
    PSRV_CREDITOR* ppCreditor
    );

VOID
SrvCreditorFree(
    PSRV_CREDITOR pCreditor
    );

// resources.c

NTSTATUS
SrvElementsResourcesInit(
    VOID
    );

VOID
SrvElementsResourcesShutdown(
    VOID
    );

// srvtimer.h

NTSTATUS
SrvTimerInit(
    IN PSRV_TIMER pTimer
    );

NTSTATUS
SrvTimerPostRequestSpecific(
    IN  PSRV_TIMER             pTimer,
    IN  LONG64                 llExpiry,
    IN  PVOID                  pUserData,
    IN  PFN_SRV_TIMER_CALLBACK pfnTimerExpiredCB,
    OUT PSRV_TIMER_REQUEST*    ppTimerRequest
    );

NTSTATUS
SrvTimerCancelRequestSpecific(
    IN  PSRV_TIMER         pTimer,
    IN  PSRV_TIMER_REQUEST pTimerRequest,
    OUT PVOID*             ppUserData
    );

NTSTATUS
SrvTimerIndicateStop(
    PSRV_TIMER pTimer
    );

VOID
SrvTimerFreeContents(
    PSRV_TIMER pTimer
    );

