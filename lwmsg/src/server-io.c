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
 *        server.c
 *
 * Abstract:
 *
 *        Multi-threaded server API
 *
 * Authors: Brian Koropoff (bkoropoff@likewisesoftware.com)
 *
 */

#include <config.h>
#include "server-private.h"

static
LWMsgStatus
lwmsg_server_io_process_tasks(
    LWMsgServer* server,
    ServerIoThread* thread,
    /* [in,out] tasks to process on call, blocked tasks on return */
    LWMsgRing* tasks,
    LWMsgBool shutdown,
    LWMsgTime* now,
    /* [out] next pending task deadline */
    LWMsgTime* next_deadline
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    LWMsgRing processing;
    LWMsgRing* iter = NULL;
    LWMsgRing* next = NULL;
    ServerTask* task = NULL;

    lwmsg_ring_init(&processing);
    lwmsg_ring_move(tasks, &processing);

    while (!lwmsg_ring_is_empty(&processing))
    {
        for (iter = processing.next; iter != &processing; iter = next)
        {
            next = iter->next;
            task = LWMSG_OBJECT_FROM_MEMBER(iter, ServerTask, ring);

            /* Detach task from ring and process it */
            lwmsg_ring_remove(&task->ring);
            BAIL_ON_ERROR(status = lwmsg_server_task_perform(
                              server,
                              thread,
                              &task,
                              shutdown,
                              now,
                              next_deadline));

            if (task)
            {
                /* We were given a task back */
                if (task->blocked)
                {
                    lwmsg_ring_insert_before(tasks, &task->ring);
                }
                else
                {
                    lwmsg_ring_insert_after(&processing, &task->ring);
                }
            }
        }
    }

error:

    return status;
}

static
LWMsgStatus
lwmsg_server_io_loop(
    ServerIoThread* thread
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    LWMsgRing tasks;
    LWMsgRing* iter = NULL;
    LWMsgRing* next = NULL;
    ServerTask* task = NULL;
    fd_set readset;
    fd_set writeset;
    int nfds = 0;
    int res = 0;
    size_t count = 0;
    LWMsgBool shutdown = LWMSG_FALSE;
    char dummy[32]; /* Dummy buffer for reading bytes out of event pipe */
    LWMsgClock clock;
    LWMsgTime now;
    LWMsgTime next_deadline;
    LWMsgTime diff;
    struct timeval timeout;
    LWMsgServer* server = thread->server;

    lwmsg_ring_init(&tasks);
    lwmsg_clock_init(&clock);

    for (;;)
    {
        /* Get current time */
        BAIL_ON_ERROR(status = lwmsg_clock_get_monotonic_time(&clock, &now));

        /* Set deadline to an invalid value */
        next_deadline.seconds = -1;
        next_deadline.microseconds = -1;

        /* Process unblocked tasks */
        BAIL_ON_ERROR(status = lwmsg_server_io_process_tasks(
                          server,
                          thread,
                          &tasks,
                          shutdown,
                          &now,
                          &next_deadline));

        /* If we are shutting down and we have no pending tasks, bail out */
        if (shutdown && lwmsg_ring_is_empty(&tasks))
        {
            break;
        }

        /* select() for blocked tasks to be ready */
        FD_ZERO(&readset);
        FD_ZERO(&writeset);
        FD_SET(thread->event[0], &readset);
        nfds = thread->event[0] + 1;

        for (iter = tasks.next; iter != &tasks; iter = next)
        {
            next = iter->next;
            task = LWMSG_OBJECT_FROM_MEMBER(iter, ServerTask, ring);

            if (task->blocked)
            {
                BAIL_ON_ERROR(status = lwmsg_server_task_prepare_select(
                                  server,
                                  task,
                                  &nfds,
                                  &readset,
                                  &writeset));
            }
        }

        if (lwmsg_time_is_positive(&next_deadline))
        {
            /* We have a pending deadline, so set up a timeout for select() */
            lwmsg_time_difference(&now, &next_deadline, &diff);
            timeout.tv_sec = diff.seconds >= 0 ? diff.seconds : 0;
            timeout.tv_usec = diff.microseconds >= 0 ? diff.microseconds : 0;
            res = select(nfds, &readset, &writeset, NULL, &timeout);
        }
        else
        {
            /* No deadline is pending, so select() indefinitely */
            res = select(nfds, &readset, &writeset, NULL, NULL);
        }

        if (res < 0)
        {
            BAIL_ON_ERROR(status = LWMSG_STATUS_SYSTEM);
        }

        /* Unblock tasks that select() indicates are ready */
        for (iter = tasks.next; iter != &tasks && res; iter = next)
        {
            next = iter->next;
            task = LWMSG_OBJECT_FROM_MEMBER(iter, ServerTask, ring);

            if (FD_ISSET(task->fd, &readset) ||
                FD_ISSET(task->fd, &writeset))
            {
                task->blocked = LWMSG_FALSE;
                res--;
            }
        }

        if (res)
        {
            /* Check for events posted to our thread */
            pthread_mutex_lock(&thread->lock);

            /* Transfer assigned tasks into local ring */
            lwmsg_ring_move((LWMsgRing*) &thread->tasks, &tasks);

            shutdown = thread->shutdown;

            count = thread->num_events;
            thread->num_events = 0;

            pthread_mutex_unlock(&thread->lock);

            /* Consume one byte in event pipe for each event we found */
            while (count)
            {
                res = (int) read(
                    thread->event[0],
                    dummy,
                    count > sizeof(dummy) ? sizeof(dummy) : count);
                if (res < 0)
                {
                    BAIL_ON_ERROR(status = LWMSG_STATUS_SYSTEM);
                }

                count -= res;
            }
        }
    }

done:

    return status;

error:

    goto done;
}

void*
lwmsg_server_io_thread(
    void* data
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    ServerIoThread* thread = data;

    BAIL_ON_ERROR(status = lwmsg_server_io_loop(thread));

done:

    return NULL;

error:

    goto done;
}
