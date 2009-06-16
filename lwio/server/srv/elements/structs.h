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
 *        defs.h
 *
 * Abstract:
 *
 *        Likewise IO (LWIO) - SRV
 *
 *        Elements
 *
 *        Structures
 *
 * Authors: Sriram Nambakam (snambakam@likewise.com)
 *
 */
#ifndef __STRUCTS_H__
#define __STRUCTS_H__

typedef struct _SRV_TIMER_REQUEST
{
	LONG                   refCount;

	struct timespec        timespan;
	PVOID                  pUserData;
	PFN_SRV_TIMER_CALLBACK pfnTimerExpiredCB;

	struct _SRV_TIMER_REQUEST* pNext;
	struct _SRV_TIMER_REQUEST* pPrev;

} SRV_TIMER_REQUEST;

typedef struct _SRV_TIMER_CONTEXT
{
    pthread_mutex_t  mutex;
    pthread_mutex_t* pMutex;

    pthread_cond_t   event;
    pthread_cond_t*  pEvent;

    PSRV_TIMER_REQUEST pRequests;

	BOOLEAN bStop;

} SRV_TIMER_CONTEXT, *PSRV_TIMER_CONTEXT;

typedef struct _SRV_TIMER
{
	pthread_t  timerThread;
	pthread_t* pTimerThread;

	SRV_TIMER_CONTEXT context;

} SRV_TIMER, *PSRV_TIMER;

typedef struct _SRV_ELEMENTS_GLOBALS
{
	pthread_mutex_t  mutex;

	SRV_TIMER timer;

} SRV_ELEMENTS_GLOBALS, *PSRV_ELEMENTS_GLOBALS;

#endif /* __STRUCTS_H__ */
