/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

/*
 * Copyright (c) Likewise Software.  All rights Reserved.
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the license, or (at
 * your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser
 * General Public License for more details.  You should have received a copy
 * of the GNU Lesser General Public License along with this program.  If
 * not, see <http://www.gnu.org/licenses/>.
 *
 * LIKEWISE SOFTWARE MAKES THIS SOFTWARE AVAILABLE UNDER OTHER LICENSING
 * TERMS AS WELL.  IF YOU HAVE ENTERED INTO A SEPARATE LICENSE AGREEMENT
 * WITH LIKEWISE SOFTWARE, THEN YOU MAY ELECT TO USE THE SOFTWARE UNDER THE
 * TERMS OF THAT SOFTWARE LICENSE AGREEMENT INSTEAD OF THE TERMS OF THE GNU
 * LESSER GENERAL PUBLIC LICENSE, NOTWITHSTANDING THE ABOVE NOTICE.  IF YOU
 * HAVE QUESTIONS, OR WISH TO REQUEST A COPY OF THE ALTERNATE LICENSING
 * TERMS OFFERED BY LIKEWISE SOFTWARE, PLEASE CONTACT LIKEWISE SOFTWARE AT
 * license@likewise.com
 */

/*
 * Copyright (C) Likewise Software. All rights reserved.
 *
 * Module Name:
 *
 *        rtlqueue.c
 *
 * Abstract:
 *
 *        Base FIFO Queue data structure
 *
 *
 * Authors: Gerald Carter <gcarter@likewise.com>
 */


#ifndef __LW_QUEUE_H__
#define __LW_QUEUE_H__

#include <lw/types.h>
#include <lw/attrs.h>
#include <lw/ntstatus.h>

typedef struct _LWRTL_QUEUE *PLWRTL_QUEUE;

typedef VOID  (*PLWRTL_QUEUE_FREE_DATA_FN)(PVOID *ppData);

NTSTATUS
LwRtlQueueInit(
    PLWRTL_QUEUE *ppNewQueue,
    DWORD dwMaxSize,
    PLWRTL_QUEUE_FREE_DATA_FN pfnFreeData
    );

NTSTATUS
LwRtlQueueSetMaxSize(
    PLWRTL_QUEUE pQueue,
    DWORD dwNewLength
    );

BOOL
LwRtlQueueIsEmpty(
    PLWRTL_QUEUE pQueue
    );

BOOL
LwRtlQueueIsFull(
    PLWRTL_QUEUE pQueue
    );

NTSTATUS
LwRtlQueueAddItem(
    PLWRTL_QUEUE pQueue,
    PVOID pItem
    );


NTSTATUS
LwRtlQueueRemoveItem(
    PLWRTL_QUEUE pQueue,
    PVOID *ppItem
    );

VOID
LwRtlQueueDestroy(
    PLWRTL_QUEUE *ppQueue
    );


#endif    /* __LW_QUEUE_H__ */


/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
