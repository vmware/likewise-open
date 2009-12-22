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
 *        task-select.c
 *
 * Abstract:
 *
 *        Task manager API (select-based implementation)
 *
 * Authors: Brian Koropoff (bkoropoff@likewisesoftware.com)
 *
 */

#include <config.h>

#include <errno.h>

#include "task-select-private.h"

#define EVENT_THREAD_COUNT 1
#define WORK_THREAD_COUNT 4
#define TASK_COMPLETE_MASK 0xFFFFFFFF

static pthread_mutex_t global_manager_lock = PTHREAD_MUTEX_INITIALIZER;
static LWMsgTaskManager* global_manager = NULL;

static
void
lwmsg_task_delete(
    SelectTask* task
    )
{
    if (task->group)
    {
        LOCK_GROUP(task->group);
        lwmsg_ring_remove(&task->group_ring);
        pthread_cond_broadcast(&task->group->event);
        UNLOCK_GROUP(task->group);
    }

    free(task);
}


static
void
lwmsg_task_signal_thread(
    SelectThread* thread
    )
{
    char c = 0;
    int res = 0;

    if (!thread->signalled)
    {
        res = write(thread->signal_fds[1], &c, sizeof(c));
        LWMSG_ASSERT(res == sizeof(c));
        thread->signalled = LWMSG_TRUE;
    }
}

static
LWMsgStatus
lwmsg_task_process_trigger(
    SelectTask* task,
    LWMsgTime* now
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    LWMsgTime new_time = {-1, -1};

    /* If task had a deadline, set the time we pass into
       the function to the time remaining */
    if (lwmsg_time_is_positive(&task->deadline))
    {
        lwmsg_time_difference(now, &task->deadline, &new_time);

        if (!lwmsg_time_is_positive(&new_time))
        {
            new_time.seconds = 0;
            new_time.microseconds = 0;
        }
    }

    BAIL_ON_ERROR(status = task->func(
                      task->funcdata,
                      task->trigger_args,
                      &task->trigger_wait,
                      &new_time));

    /* If the function gave us a valid time,
       update the task deadline */
    if (lwmsg_time_is_positive(&new_time))
    {
        lwmsg_time_sum(now, &new_time, &task->deadline);
    }
    else
    {
        task->deadline.seconds = -1;
        task->deadline.microseconds = -1;
    }

error:

    return status;
}

static
void
lwmsg_task_update_trigger_wait(
    SelectTask* task,
    int* nfds,
    fd_set* read_set,
    fd_set* write_set,
    fd_set* except_set,
    LWMsgTime* next_deadline
    )
{
    if (task->trigger_wait & LWMSG_TASK_TRIGGER_FD_READABLE)
    {
        FD_SET(task->fd, read_set);
        if (task->fd >= *nfds)
        {
            *nfds = task->fd + 1;
        }
    }

    if (task->trigger_wait & LWMSG_TASK_TRIGGER_FD_WRITABLE)
    {
        FD_SET(task->fd, write_set);
        if (task->fd >= *nfds)
        {
            *nfds = task->fd + 1;
        }
    }

    if (task->trigger_wait & LWMSG_TASK_TRIGGER_FD_EXCEPTION)
    {
        FD_SET(task->fd, except_set);
        if (task->fd >= *nfds)
        {
            *nfds = task->fd + 1;
        }
    }

    if (task->trigger_wait & LWMSG_TASK_TRIGGER_TIME &&
        lwmsg_time_is_positive(&task->deadline))
    {
        if (!lwmsg_time_is_positive(next_deadline) ||
            lwmsg_time_compare(next_deadline, &task->deadline) == LWMSG_TIME_GREATER)
        {
            *next_deadline = task->deadline;
        }
    }
}

static
void
lwmsg_task_update_trigger_set(
    SelectTask* task,
    fd_set* read_set,
    fd_set* write_set,
    fd_set* except_set,
    LWMsgTime* now
    )
{
    if (task->fd >= 0)
    {
        if (FD_ISSET(task->fd, read_set))
        {
            FD_CLR(task->fd, read_set);
            task->trigger_set |= LWMSG_TASK_TRIGGER_FD_READABLE;
        }

        if (FD_ISSET(task->fd, write_set))
        {
            FD_CLR(task->fd, write_set);
            task->trigger_set |= LWMSG_TASK_TRIGGER_FD_WRITABLE;
        }

        if (FD_ISSET(task->fd, except_set))
        {
            FD_CLR(task->fd, except_set);
            task->trigger_set |= LWMSG_TASK_TRIGGER_FD_EXCEPTION;
        }
    }

    /* If the task's deadline has expired, set the time trigger bit */
    if (task->trigger_wait & LWMSG_TASK_TRIGGER_TIME &&
        lwmsg_time_is_positive(&task->deadline) &&
        lwmsg_time_compare(now, &task->deadline) >= LWMSG_TIME_EQUAL)
    {
        task->trigger_set |= LWMSG_TASK_TRIGGER_TIME;
    }
}

static
LWMsgStatus
lwmsg_task_sleep(
    LWMsgClock* clock,
    int nfds,
    fd_set* read_set,
    fd_set* write_set,
    fd_set* except_set,
    LWMsgTime* next_deadline,
    LWMsgTime* now,
    int* ready
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    LWMsgTime diff;
    struct timeval timeout;
    int my_ready = 0;

    if (lwmsg_time_is_positive(next_deadline))
    {
        /* We have a pending deadline, so set up a timeout for select() */
        do
        {
            lwmsg_time_difference(now, next_deadline, &diff);
            timeout.tv_sec = diff.seconds >= 0 ? diff.seconds : 0;
            timeout.tv_usec = diff.microseconds >= 0 ? diff.microseconds : 0;

            my_ready = select(nfds, read_set, write_set, except_set, &timeout);

            if (my_ready < 0 && errno == EINTR)
            {
                /* Update current time so the next timeout calculation is correct */
                BAIL_ON_ERROR(status = lwmsg_clock_get_monotonic_time(clock, now));
            }
        } while (my_ready < 0 && errno == EINTR);
    }
    else
    {
        /* No deadline is pending, so select() indefinitely */
        do
        {
            my_ready = select(nfds, read_set, write_set, except_set, NULL);
        } while (my_ready < 0 && errno == EINTR);
    }

    if (my_ready < 0)
    {
        BAIL_ON_ERROR(status = lwmsg_error_map_errno(errno));
    }

    *ready = my_ready;

error:

    return status;
}

static
LWMsgStatus
lwmsg_task_event_loop(
    SelectThread* thread
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    LWMsgRing tasks;
    LWMsgRing triggered;
    LWMsgRing* ring = NULL;
    LWMsgRing* next = NULL;
    SelectTask* task = NULL;
    LWMsgClock clock;
    LWMsgTime now;
    LWMsgTime next_deadline;
    fd_set read_set;
    fd_set write_set;
    fd_set except_set;
    int ready = 0;
    int nfds = 0;
    char c = 0;
    int res = 0;
    LWMsgBool shutdown = LWMSG_FALSE;
    LWMsgTaskGroup* group = NULL;

    lwmsg_clock_init(&clock);

    lwmsg_ring_init(&tasks);
    lwmsg_ring_init(&triggered);

    FD_ZERO(&read_set);
    FD_ZERO(&write_set);
    FD_ZERO(&except_set);

    LOCK_THREAD(thread);

    while (!shutdown || !lwmsg_ring_is_empty(&tasks))
    {
        /* Reset next deadline */
        next_deadline.seconds = -1;
        next_deadline.microseconds = -1;
        nfds = 0;

        /* Get current time for this iteration */
        BAIL_ON_ERROR(status = lwmsg_clock_get_monotonic_time(&clock, &now));

        /* Figure out which tasks have been triggered */
        for (ring = tasks.next; ring != &tasks; ring = next)
        {
            next = ring->next;

            task = LWMSG_OBJECT_FROM_MEMBER(ring, SelectTask, event_ring);

            /* Update trigger set with results from select() */
            lwmsg_task_update_trigger_set(
                task,
                &read_set,
                &write_set,
                &except_set,
                &now);

            if ((task->trigger_wait | LWMSG_TASK_TRIGGER_EXPLICIT) & task->trigger_set)
            {
                /* Put task on a separate list to run its trigger function */
                lwmsg_ring_remove(&task->event_ring);
                lwmsg_ring_insert_before(&triggered, &task->event_ring);
                /* Copy the trigger set into a separate field that can
                   be safely accessed outside the thread lock */
                task->trigger_args = task->trigger_set;
            }
            else
            {
                /* Update select parameters to wait for task to trigger */
                lwmsg_task_update_trigger_wait(
                    task,
                    &nfds,
                    &read_set,
                    &write_set,
                    &except_set,
                    &next_deadline);
            }

            /* Turn off trigger bits now that we have examined them */
            task->trigger_set &= (LWMSG_TASK_TRIGGER_CANCEL);
        }

        UNLOCK_THREAD(thread);

        /* Process triggered tasks */
        for (ring = triggered.next; ring != &triggered; ring = next)
        {
            next = ring->next;

            task = LWMSG_OBJECT_FROM_MEMBER(ring, SelectTask, event_ring);

            lwmsg_ring_remove(&task->event_ring);

            // FIXME: relock thread?
            BAIL_ON_ERROR(status = lwmsg_task_process_trigger(task, &now));

            /* If task is still waiting to trigger, update select parameters
               and put it back in the task list */
            if (task->trigger_wait)
            {
                lwmsg_task_update_trigger_wait(
                    task,
                    &nfds,
                    &read_set,
                    &write_set,
                    &except_set,
                    &next_deadline);
                lwmsg_ring_insert_before(&tasks, &task->event_ring);
            }
            else
            {
                /* Task is complete, notify waiters or delete if we held
                   the last reference */
                LOCK_THREAD(thread);
                if (--task->refs)
                {
                    /* Save a reference to the task's group, if any,
                       because we can't safely access the task structure
                       after unlocking the thread (we dropped our reference
                       so it could be freed by another thread) */
                    group = task->group;

                    task->trigger_set = TASK_COMPLETE_MASK;
                    pthread_cond_broadcast(&thread->event);
                    UNLOCK_THREAD(thread);

                    if (group)
                    {
                        LOCK_GROUP(group);
                        pthread_cond_broadcast(&group->event);
                        UNLOCK_GROUP(group);
                    }
                }
                else
                {
                    UNLOCK_THREAD(thread);
                    lwmsg_task_delete(task);
                }
            }
        }

        if (!shutdown)
        {
            /* Also wait for a poke on the thread's signal fd */
            FD_SET(thread->signal_fds[0], &read_set);
            if (thread->signal_fds[0] >= nfds)
            {
                nfds = thread->signal_fds[0] + 1;
            }
        }

        if (nfds)
        {
            /* Wait for a task to be triggered */
            BAIL_ON_ERROR(status = lwmsg_task_sleep(
                              &clock,
                              nfds,
                              &read_set,
                              &write_set,
                              &except_set,
                              &next_deadline,
                              &now,
                              &ready));
        }

        LOCK_THREAD(thread);

        /* Check for a signal to the thread */
        if (FD_ISSET(thread->signal_fds[0], &read_set))
        {
            FD_CLR(thread->signal_fds[0], &read_set);
            thread->signalled = LWMSG_FALSE;

            res = read(thread->signal_fds[0], &c, sizeof(c));
            LWMSG_ASSERT(res == sizeof(c));

            /* Move all tasks in queue into local task list */
            lwmsg_ring_move(&thread->tasks, &tasks);

            if (thread->shutdown && !shutdown)
            {
                shutdown = thread->shutdown;

                /* Cancel all outstanding tasks */
                for (ring = tasks.next; ring != &tasks; ring = ring->next)
                {
                    task = LWMSG_OBJECT_FROM_MEMBER(ring, SelectTask, event_ring);
                    task->trigger_set |= LWMSG_TASK_TRIGGER_CANCEL | LWMSG_TASK_TRIGGER_EXPLICIT;
                }
            }
        }
    }

error:

    UNLOCK_THREAD(thread);

    return status;
}

static
void*
lwmsg_task_event_thread(
    void* data
    )
{
    lwmsg_task_event_loop((SelectThread*) data);

    return NULL;
}

static
LWMsgStatus
lwmsg_task_work_loop(
    WorkItemThread* thread
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    LWMsgRing* ring = NULL;
    WorkItem* item = NULL;

    LOCK_MANAGER(thread->manager);

    for(;;)
    {
        while (!thread->manager->shutdown && lwmsg_ring_is_empty(&thread->manager->work_items))
        {
            pthread_cond_wait(&thread->manager->event, &thread->manager->lock);
        }

        thread->manager->work_threads_free_count--;

        if (thread->manager->shutdown)
        {
            break;
        }

        lwmsg_ring_dequeue(&thread->manager->work_items, &ring);

        UNLOCK_MANAGER(thread->manager);

        item = LWMSG_OBJECT_FROM_MEMBER(ring, WorkItem, ring);

        item->func(item->data);
        free(item);

        LOCK_MANAGER(thread->manager);

        thread->manager->work_threads_free_count++;
    }

    UNLOCK_MANAGER(thread->manager);

    return status;
}

static
void*
lwmsg_task_work_thread(
    void* data
    )
{
    lwmsg_task_work_loop((WorkItemThread*) data);

    return NULL;
}

static
void*
lwmsg_task_temporary_work_thread(
    void* data
    )
{
    WorkItem* item = (WorkItem*) data;

    item->func(item->data);

    free(item);

    return NULL;
}

LWMsgStatus
lwmsg_task_new(
    LWMsgTaskManager* manager,
    LWMsgTaskGroup* group,
    LWMsgTaskFunction func,
    void* data,
    LWMsgTask** task
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    SelectTask* my_task = NULL;

    BAIL_ON_ERROR(status = LWMSG_ALLOC(&my_task));

    lwmsg_ring_init(&my_task->group_ring);
    lwmsg_ring_init(&my_task->event_ring);
    my_task->manager = manager;
    my_task->group = group;
    my_task->refs = 2;
    my_task->func = func;
    my_task->funcdata = data;
    my_task->fd = -1;
    my_task->trigger_set = LWMSG_TASK_TRIGGER_INIT;
    my_task->trigger_wait = LWMSG_TASK_TRIGGER_EXPLICIT;
    my_task->deadline.seconds = -1;
    my_task->deadline.microseconds = -1;

    LOCK_MANAGER(manager);
    my_task->thread = &manager->event_threads[manager->next_event_thread];
    manager->next_event_thread = (manager->next_event_thread + 1) % manager->event_threads_count;
    UNLOCK_MANAGER(manager);

    if (group)
    {
        LOCK_GROUP(group);
        lwmsg_ring_insert_before(&group->tasks, &my_task->group_ring);
        UNLOCK_GROUP(group);
    }

    LOCK_THREAD(my_task->thread);
    lwmsg_ring_insert_before(&my_task->thread->tasks, &my_task->event_ring);
    /* It's not necessary to signal the thread about the new task here
       since it won't be run anyway */
    UNLOCK_THREAD(my_task->thread);

    *task = my_task;

error:

    return status;
}

LWMsgStatus
lwmsg_task_group_new(
    LWMsgTaskManager* manager,
    LWMsgTaskGroup** group
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    SelectTaskGroup* my_group = NULL;

    BAIL_ON_ERROR(status = LWMSG_ALLOC(&my_group));
    BAIL_ON_ERROR(status = lwmsg_error_map_errno(pthread_mutex_init(&my_group->lock, NULL)));
    BAIL_ON_ERROR(status = lwmsg_error_map_errno(pthread_cond_init(&my_group->event, NULL)));

    lwmsg_ring_init(&my_group->tasks);
    my_group->manager = manager;

    *group = my_group;

error:

    return status;
}

void
lwmsg_task_release(
    LWMsgTask* task
    )
{
    int refs = 0;
    LOCK_THREAD(task->thread);
    refs = --task->refs;
    UNLOCK_THREAD(task->thread);

    if (refs == 0)
    {
        lwmsg_task_delete(task);
    }
}

void
lwmsg_task_group_delete(
    LWMsgTaskGroup* group
    )
{
    pthread_mutex_destroy(&group->lock);
    pthread_cond_destroy(&group->event);
    free(group);
}

void
lwmsg_task_set_trigger_fd(
    LWMsgTask* task,
    int fd
    )
{
    task->fd = fd;
}

void
lwmsg_task_wake(
    LWMsgTask* task
    )
{
    LOCK_THREAD(task->thread);
    task->trigger_set |= LWMSG_TASK_TRIGGER_EXPLICIT;
    lwmsg_task_signal_thread(task->thread);
    UNLOCK_THREAD(task->thread);
}

void
lwmsg_task_cancel(
    LWMsgTask* task
    )
{
    LOCK_THREAD(task->thread);

    task->trigger_set |= LWMSG_TASK_TRIGGER_EXPLICIT | LWMSG_TASK_TRIGGER_CANCEL;
    lwmsg_task_signal_thread(task->thread);

    UNLOCK_THREAD(task->thread);
}

void
lwmsg_task_wait(
    LWMsgTask* task
    )
{
    LOCK_THREAD(task->thread);

    while (task->trigger_set != TASK_COMPLETE_MASK)
    {
        pthread_cond_wait(&task->thread->event, &task->thread->lock);
    }

    UNLOCK_THREAD(task->thread);
}

void
lwmsg_task_group_wake(
    LWMsgTaskGroup* group
    )
{
    LWMsgRing* ring = NULL;
    LWMsgTask* task = NULL;

    LOCK_GROUP(group);

    for (ring = group->tasks.next; ring != &group->tasks; ring = ring->next)
    {
        task = LWMSG_OBJECT_FROM_MEMBER(ring, SelectTask, group_ring);

        lwmsg_task_wake(task);
    }

    UNLOCK_GROUP(group);
}

void
lwmsg_task_group_cancel(
    LWMsgTaskGroup* group
    )
{
    LWMsgRing* ring = NULL;
    LWMsgTask* task = NULL;

    LOCK_GROUP(group);

    for (ring = group->tasks.next; ring != &group->tasks; ring = ring->next)
    {
        task = LWMSG_OBJECT_FROM_MEMBER(ring, SelectTask, group_ring);

        lwmsg_task_cancel(task);
    }

    UNLOCK_GROUP(group);
}

void
lwmsg_task_group_wait(
    LWMsgTaskGroup* group
    )
{
    LWMsgRing* ring = NULL;
    LWMsgTask* task = NULL;
    LWMsgBool still_alive = LWMSG_TRUE;

    LOCK_GROUP(group);

    while (still_alive)
    {
        still_alive = LWMSG_FALSE;

        for (ring = group->tasks.next; !still_alive && ring != &group->tasks; ring = ring->next)
        {
            task = LWMSG_OBJECT_FROM_MEMBER(ring, SelectTask, group_ring);

            LOCK_THREAD(task->thread);
            if (task->trigger_set != TASK_COMPLETE_MASK)
            {
                still_alive = LWMSG_TRUE;
            }
            UNLOCK_THREAD(task->thread);
        }

        if (still_alive)
        {
            pthread_cond_wait(&group->event, &group->lock);
        }
    }

    UNLOCK_GROUP(group);
}

static
LWMsgStatus
lwmsg_task_select_thread_init(
    SelectThread* thread
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;

    BAIL_ON_ERROR(status = lwmsg_error_map_errno(pthread_mutex_init(&thread->lock, NULL)));
    BAIL_ON_ERROR(status = lwmsg_error_map_errno(pthread_cond_init(&thread->event, NULL)));

    if (pipe(thread->signal_fds) < 0)
    {
        BAIL_ON_ERROR(status = lwmsg_error_map_errno(errno));
    }

    lwmsg_ring_init(&thread->tasks);

    BAIL_ON_ERROR(status = pthread_create(
                      &thread->thread,
                      NULL,
                      lwmsg_task_event_thread,
                      thread));

error:

    return status;
}

static
void
lwmsg_task_select_thread_destroy(
    SelectThread* thread
    )
{
    pthread_mutex_destroy(&thread->lock);
    pthread_cond_destroy(&thread->event);
}

static
LWMsgStatus
lwmsg_task_work_thread_init(
    SelectManager* manager,
    WorkItemThread* thread
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;

    thread->manager = manager;

    BAIL_ON_ERROR(status = pthread_create(
                      &thread->thread,
                      NULL,
                      lwmsg_task_work_thread,
                      thread));

error:

    return status;
}

static
LWMsgStatus
lwmsg_task_select_manager_new(
    LWMsgTaskManager** manager
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    LWMsgTaskManager* my_manager = NULL;
    int i = 0;

    BAIL_ON_ERROR(status = LWMSG_ALLOC(&my_manager));

    lwmsg_ring_init(&my_manager->work_items);

    BAIL_ON_ERROR(status = lwmsg_error_map_errno(pthread_mutex_init(&my_manager->lock, NULL)));
    BAIL_ON_ERROR(status = lwmsg_error_map_errno(pthread_cond_init(&my_manager->event, NULL)));

    my_manager->event_threads_count = EVENT_THREAD_COUNT;
    my_manager->next_event_thread = 0;
    BAIL_ON_ERROR(status = LWMSG_ALLOC_ARRAY(my_manager->event_threads_count, &my_manager->event_threads));

    for (i = 0; i < my_manager->event_threads_count; i++)
    {
        BAIL_ON_ERROR(status = lwmsg_task_select_thread_init(&my_manager->event_threads[i]));
    }

    my_manager->work_threads_count = WORK_THREAD_COUNT;
    my_manager->work_threads_free_count = WORK_THREAD_COUNT;
    BAIL_ON_ERROR(status = LWMSG_ALLOC_ARRAY(my_manager->work_threads_count, &my_manager->work_threads));

    for (i = 0; i < my_manager->work_threads_count; i++)
    {
        BAIL_ON_ERROR(status = lwmsg_task_work_thread_init(my_manager, &my_manager->work_threads[i]));
    }

    my_manager->refs = 1;

    *manager = my_manager;

error:

    return status;
}

static
void
lwmsg_task_select_manager_delete(
    LWMsgTaskManager* manager
    )
{
    SelectThread* thread = NULL;
    int i = 0;

    LOCK_MANAGER(manager);
    manager->shutdown = LWMSG_TRUE;
    pthread_cond_broadcast(&manager->event);
    UNLOCK_MANAGER(manager);

    if (manager->event_threads)
    {
        for (i = 0; i < manager->event_threads_count; i++)
        {
            thread = &manager->event_threads[i];
            LOCK_THREAD(thread);
            thread->shutdown = LWMSG_TRUE;
            lwmsg_task_signal_thread(thread);
            UNLOCK_THREAD(thread);
            pthread_join(thread->thread, NULL);
            lwmsg_task_select_thread_destroy(thread);
        }

        free(manager->event_threads);
    }

    if (manager->work_threads)
    {
        for (i = 0; i < manager->work_threads_count; i++)
        {
            pthread_join(manager->work_threads[i].thread, NULL);
        }

        free(manager->work_threads);
    }

    pthread_mutex_destroy(&manager->lock);

    free(manager);
}

LWMsgStatus
lwmsg_task_dispatch_work_item(
    LWMsgTaskManager* manager,
    LWMsgWorkItemFunction func,
    void* data
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    WorkItem* item = NULL;
    pthread_t thread;

    BAIL_ON_ERROR(status = LWMSG_ALLOC(&item));
    lwmsg_ring_init(&item->ring);
    item->func = func;
    item->data = data;

    LOCK_MANAGER(manager);

    if (manager->work_threads_free_count == 0)
    {
        BAIL_ON_ERROR(status = lwmsg_error_map_errno(
                          pthread_create(
                              &thread,
                              NULL,
                              lwmsg_task_temporary_work_thread,
                              item)));
        item = NULL;
        BAIL_ON_ERROR(status = lwmsg_error_map_errno(pthread_detach(thread)));

    }
    else
    {
        lwmsg_ring_enqueue(&manager->work_items, &item->ring);
        pthread_cond_signal(&manager->event);
        item = NULL;
    }

error:

    UNLOCK_MANAGER(manager);

    if (item)
    {
        free(item);
    }

    return status;
}

LWMsgStatus
lwmsg_task_acquire_manager(
    LWMsgTaskManager** manager
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;

    pthread_mutex_lock(&global_manager_lock);

    if (global_manager)
    {
        global_manager->refs++;
        *manager = global_manager;
    }
    else
    {
        BAIL_ON_ERROR(status = lwmsg_task_select_manager_new(&global_manager));

        *manager = global_manager;
    }

error:

    pthread_mutex_unlock(&global_manager_lock);

    return status;
}

void
lwmsg_task_release_manager(
    LWMsgTaskManager* manager
    )
{
    pthread_mutex_lock(&global_manager_lock);

    if (--manager->refs == 0)
    {
        if (manager == global_manager)
        {
            global_manager = NULL;
        }

        lwmsg_task_select_manager_delete(manager);
    }

    pthread_mutex_unlock(&global_manager_lock);
}
