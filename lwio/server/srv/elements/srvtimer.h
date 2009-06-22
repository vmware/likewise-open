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
 *        srvtimer.h
 *
 * Abstract:
 *
 *        Likewise IO (LWIO) - SRV
 *
 *        Elements
 *
 *        Timer
 *
 * Authors: Sriram Nambakam (snambakam@likewise.com)
 */

#ifndef __SRV_TIMER_H__
#define __SRV_TIMER_H__

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
    IN  PSRV_TIMER_REQUEST pTimerRequest
    );

VOID
SrvTimerIndicateStop(
    PSRV_TIMER pTimer
    );

VOID
SrvTimerFreeContents(
    PSRV_TIMER pTimer
    );

#endif /* __SRV_TIMER_H__ */
