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
 *        Task manager API (private header -- select-based implementation)
 *
 * Authors: Brian Koropoff (bkoropoff@likewise.com)
 *
 */
#ifndef __LWMSG_TASK_SELECT_PRIVATE_H__
#define __LWMSG_TASK_SELECT_PRIVATE_H__

#include <lwmsg/task.h>
#include <lwmsg/common.h>

#include "util-private.h"

#include <pthread.h>

typedef struct SelectThread
{
    pthread_t thread;
    pthread_mutex_t lock;
    pthread_cond_t event;
    int signal_fds[2];
    LWMsgRing tasks;
    LWMsgBool signalled;
    LWMsgBool shutdown;
} SelectThread;

typedef struct WorkItemThread
{
    LWMsgTaskManager* manager;
    pthread_t thread;
} WorkItemThread;

typedef struct LWMsgTask
{
    /* Owning task manager */
    LWMsgTaskManager* manager;
    /* Owning thread */
    SelectThread* thread;
    /* Owning group */
    LWMsgTaskGroup* group;
    /* Ref count */
    int volatile refs;
    /* Trigger conditions task is waiting for */
    LWMsgTaskTrigger trigger_wait;
    /* Trigger conditions that have been satisfied */
    LWMsgTaskTrigger volatile trigger_set;
    /* Trigger conditions that will be passed to func() */
    LWMsgTaskTrigger trigger_args;
    /* Absolute time of next time wake event */
    LWMsgTime deadline;
    /* Callback function and data */
    LWMsgTaskFunction func;
    void* funcdata;
    /* File descriptor for fd-based events */
    int fd;
    /* Link to siblings in task group */
    LWMsgRing group_ring;
    /* Link to siblings in event loop */
    LWMsgRing event_ring;
} SelectTask;

typedef struct WorkItem
{
    LWMsgWorkItemFunction func;
    void* data;
    LWMsgRing ring;
} WorkItem;

typedef struct LWMsgTaskGroup
{
    LWMsgTaskManager* manager;
    LWMsgRing tasks;
    pthread_mutex_t lock;
    pthread_cond_t event;
} SelectTaskGroup;

typedef struct LWMsgTaskManager
{
    int refs;
    SelectThread* event_threads;
    int event_threads_count;
    int next_event_thread;
    WorkItemThread* work_threads;
    int work_threads_count;
    LWMsgRing work_items;
    LWMsgBool volatile shutdown;
    pthread_mutex_t lock;
    pthread_cond_t event;
} SelectManager;

/*
 * Lock order discipline:
 *
 * Always lock manager before locking a thread
 * Always lock a group before locking a thread
 */

#define LOCK_THREAD(st) (pthread_mutex_lock(&(st)->lock))
#define UNLOCK_THREAD(st) (pthread_mutex_unlock(&(st)->lock))
#define LOCK_GROUP(g) (pthread_mutex_lock(&(g)->lock))
#define UNLOCK_GROUP(g) (pthread_mutex_unlock(&(g)->lock))
#define LOCK_MANAGER(m) (pthread_mutex_lock(&(m)->lock))
#define UNLOCK_MANAGER(m) (pthread_mutex_unlock(&(m)->lock))

#endif
