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
 * license@likewisesoftware.com
 */

/*
 * Module Name:
 *
 *        task.h
 *
 * Abstract:
 *
 *        Task manager API (public header)
 *
 * Authors: Brian Koropoff (bkoropoff@likewise.com)
 *
 */
#ifndef __LWMSG_TASK_H__
#define __LWMSG_TASK_H__

#include <lwmsg/status.h>
#include <lwmsg/time.h>

typedef struct LWMsgTaskManager LWMsgTaskManager;
typedef struct LWMsgTaskGroup LWMsgTaskGroup;
typedef struct LWMsgTask LWMsgTask;

typedef enum LWMsgTaskTrigger
{
    LWMSG_TASK_TRIGGER_INIT         = 0x01,
    LWMSG_TASK_TRIGGER_EXPLICIT     = 0x02,
    LWMSG_TASK_TRIGGER_CANCEL       = 0x04,
    LWMSG_TASK_TRIGGER_TIME         = 0x08,
    LWMSG_TASK_TRIGGER_FD_READABLE  = 0x10,
    LWMSG_TASK_TRIGGER_FD_WRITABLE  = 0x20,
    LWMSG_TASK_TRIGGER_FD_EXCEPTION = 0x40
} LWMsgTaskTrigger;

typedef LWMsgStatus (*LWMsgTaskFunction)(
    void* task_data,
    LWMsgTaskTrigger trigger,
    LWMsgTaskTrigger* new_trigger,
    LWMsgTime* new_time
    );

typedef void (*LWMsgWorkItemFunction)(
    void* work_data
    );

LWMsgStatus
lwmsg_task_new(
    LWMsgTaskManager* manager,
    LWMsgTaskGroup* group,
    LWMsgTaskFunction func,
    void* data,
    LWMsgTask** task
    );

LWMsgStatus
lwmsg_task_group_new(
    LWMsgTaskManager* manager,
    LWMsgTaskGroup** group
    );

void
lwmsg_task_release(
    LWMsgTask* task
    );

void
lwmsg_task_group_delete(
    LWMsgTaskGroup* group
    );

void
lwmsg_task_set_trigger_fd(
    LWMsgTask* task,
    int fd
    );

void
lwmsg_task_wake(
    LWMsgTask* task
    );

void
lwmsg_task_cancel(
    LWMsgTask* task
    );

void
lwmsg_task_wait(
    LWMsgTask* task
    );

void
lwmsg_task_group_wake(
    LWMsgTaskGroup* group
    );

void
lwmsg_task_group_cancel(
    LWMsgTaskGroup* group
    );

void
lwmsg_task_group_wait(
    LWMsgTaskGroup* group
    );

LWMsgStatus
lwmsg_task_dispatch_work_item(
    LWMsgTaskManager* manager,
    LWMsgWorkItemFunction func,
    void* data
    );

LWMsgStatus
lwmsg_task_acquire_manager(
    LWMsgTaskManager** manager
    );

void
lwmsg_task_release_manager(
    LWMsgTaskManager* manager
    );

#endif
