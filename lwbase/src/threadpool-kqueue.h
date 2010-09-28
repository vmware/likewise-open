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
 * HAVE QUESTIONS, OR WISHTO REQUEST A COPY OF THE ALTERNATE LICENSING
 * TERMS OFFERED BY LIKEWISE SOFTWARE, PLEASE CONTACT LIKEWISE SOFTWARE AT
 * license@likewisesoftware.com
 */

/*
 * Module Name:
 *
 *        threadpool-internal.h
 *
 * Abstract:
 *
 *        Thread pool API
 *
 * Authors: Brian Koropoff (bkoropoff@likewise.com)
 *
 */

#ifndef __LWBASE_THREADPOOL_INTERNAL_H__
#define __LWBASE_THREADPOOL_INTERNAL_H__

#include <lw/threadpool.h>
#include <lw/rtlgoto.h>
#include <sys/types.h>
#include <sys/event.h>
#include <sys/time.h>
#include <sys/param.h>

#include "threadpool-common.h"

#define TASK_COMPLETE_MASK 0xFFFFFFFF
#define COMMANDS_PER_FD 2

typedef struct _KQUEUE_COMMANDS
{
  struct kevent* pCommands;
  /* Command slots used */
  ULONG ulCommandCount;
  /* Command slots allocated */
  ULONG ulCommandCapacity;
  /*
   * Maximum command slots we could possibly need
   * based on the number of tasks with active fd events.
   */
  ULONG ulCommandMax;
} KQUEUE_COMMANDS, *PKQUEUE_COMMANDS;

typedef struct _KQUEUE_THREAD
{
    PLW_THREAD_POOL pPool;
    pthread_t Thread;
    pthread_mutex_t Lock;
    pthread_cond_t Event;
    /* Self pipe for explicitly waking the thread */
    int SignalFds[2];
    int KqueueFd;
    KQUEUE_COMMANDS Commands;
    RING Tasks;
    /* Thread load (protected by thread pool lock) */
    ULONG volatile ulLoad;
    BOOLEAN volatile bSignalled;
    BOOLEAN volatile bShutdown;
} KQUEUE_THREAD, *PKQUEUE_THREAD;

typedef struct _LW_TASK
{
    /* Owning thread */
    PKQUEUE_THREAD pThread;
    /* Owning group */
    PLW_TASK_GROUP pGroup;
    /* Ref count (protected by thread lock) */
    ULONG volatile ulRefCount;
    /* Events task is waiting for (owned by thread) */
    LW_TASK_EVENT_MASK EventWait;
    /* Last set of events task waited for (owned by thread) */
    LW_TASK_EVENT_MASK EventLastWait;
    /* Events that will be passed on the next wakeup (owned by thread) */
    LW_TASK_EVENT_MASK EventArgs;
    /* Event conditions signalled between owning thread and
       external callers (protected by thread lock) */
    LW_TASK_EVENT_MASK volatile EventSignal;
    /* Absolute time of next time wake event (owned by thread) */
    LONG64 llDeadline;
    /* Callback function and context (immutable) */
    LW_TASK_FUNCTION pfnFunc;
    PVOID pFuncContext;
    /* File descriptor for fd-based events (owned by thread) */
    int Fd;
    /* Link to siblings in task group (protected by group lock) */
    RING GroupRing;
    /* Link to siblings in scheduler queue (owned by thread) */
    RING QueueRing;
    /* Link to siblings in signal queue (protected by thread lock) */
    RING SignalRing;
} KQUEUE_TASK, *PKQUEUE_TASK;

typedef struct _LW_TASK_GROUP
{
    PLW_THREAD_POOL pPool;
    RING Tasks;
    pthread_mutex_t Lock;
    pthread_cond_t Event;
    unsigned bCancelled:1;
} KQUEUE_TASK_GROUP, *PKQUEUE_TASK_GROUP;

typedef struct _LW_THREAD_POOL
{
    PLW_THREAD_POOL pDelegate;
    PKQUEUE_THREAD pEventThreads;
    ULONG ulEventThreadCount;
    BOOLEAN volatile bShutdown;
    pthread_mutex_t Lock;
    pthread_cond_t Event;
    LW_WORK_THREADS WorkThreads;
} KQUEUE_POOL, *PKQUEUE_POOL;

/*
 * Lock order discipline:
 *
 * Always lock manager before locking a thread
 * Always lock a group before locking a thread
 * Always lock threads at a lower index first
 */

#define LOCK_THREAD(st) (pthread_mutex_lock(&(st)->Lock))
#define UNLOCK_THREAD(st) (pthread_mutex_unlock(&(st)->Lock))
#define LOCK_GROUP(g) (pthread_mutex_lock(&(g)->Lock))
#define UNLOCK_GROUP(g) (pthread_mutex_unlock(&(g)->Lock))
#define LOCK_POOL(m) (pthread_mutex_lock(&(m)->Lock))
#define UNLOCK_POOL(m) (pthread_mutex_unlock(&(m)->Lock))

#endif
