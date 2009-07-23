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
 *        client.c
 *
 * Abstract:
 *
 *        Multi-threaded client API
 *
 * Authors: Brian Koropoff (bkoropoff@likewisesoftware.com)
 *
 */
#include "client-private.h"
#include "util-private.h"
#include "session-private.h"
#include "assoc-private.h"
#include "protocol-private.h"

#include <stdlib.h>
#include <string.h>

static void
lwmsg_client_lock(
    LWMsgClient* client
    )
{
    pthread_mutex_lock(&client->lock);
}

static void
lwmsg_client_unlock(
    LWMsgClient* client
    )
{
    pthread_mutex_unlock(&client->lock);
}

static void
lwmsg_client_wait(
    LWMsgClient* client
    )
{
    pthread_cond_wait(&client->event, &client->lock);
}

static void
lwmsg_client_signal(
    LWMsgClient* client
    )
{
    pthread_cond_signal(&client->event);
}

LWMsgStatus
lwmsg_client_new(
    const LWMsgContext* context,
    LWMsgProtocol* protocol,
    LWMsgClient** out_client
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    LWMsgClient* client = NULL;

    client = calloc(1, sizeof(*client));

    if (!client)
    {
        BAIL_ON_ERROR(status = LWMSG_STATUS_MEMORY);
    }

    if (pthread_mutex_init(&client->lock, NULL))
    {
        BAIL_ON_ERROR(status = LWMSG_STATUS_SYSTEM);
    }

    if (pthread_cond_init(&client->event, NULL))
    {
        BAIL_ON_ERROR(status = LWMSG_STATUS_SYSTEM);
    }

    BAIL_ON_ERROR(status = lwmsg_shared_session_manager_new(&client->manager));

    client->context = context;
    client->protocol = protocol;
    client->call_pool_capacity = 4;
    client->call_pool_created = 0;
    client->call_pool_available = 0;
    client->call_pool = calloc(client->call_pool_capacity, sizeof(*client->call_pool));

    if (!client->call_pool)
    {
        BAIL_ON_ERROR(status = LWMSG_STATUS_MEMORY);
    }

    *out_client = client;

done:

    return status;

error:

    if (client)
    {
        if (client->manager)
        {
            lwmsg_session_manager_delete(client->manager);
        }

        if (client->call_pool)
        {
            free(client->call_pool);
        }

        free(client);
    }

    goto done;
}

LWMsgStatus
lwmsg_client_set_endpoint(
    LWMsgClient* client,
    LWMsgConnectionMode mode,
    const char* endpoint
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;

    lwmsg_client_lock(client);

    if (client->call_pool_created)
    {
        BAIL_ON_ERROR(status = LWMSG_STATUS_INVALID_STATE);
    }

    client->endpoint = strdup(endpoint);

    if (!client->endpoint)
    {
        BAIL_ON_ERROR(status = LWMSG_STATUS_MEMORY);
    }

    client->mode = mode;

error:

    lwmsg_client_unlock(client);

    return status;
}

static
LWMsgStatus
lwmsg_client_call_establish_assoc(
    ClientCall* call
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;

    if (!call->assoc)
    {
        BAIL_ON_ERROR(status = lwmsg_connection_new(call->client->context, call->client->protocol, &call->assoc));
        BAIL_ON_ERROR(status = lwmsg_assoc_set_session_manager(call->assoc, call->client->manager));
        BAIL_ON_ERROR(status = lwmsg_connection_set_endpoint(call->assoc, call->client->mode, call->client->endpoint));
        BAIL_ON_ERROR(status = lwmsg_assoc_establish(call->assoc));
    }

error:

    return status;
}

LWMsgStatus
lwmsg_client_acquire_call(
    LWMsgClient* client,
    LWMsgCall** out_call
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    ClientCall* call = NULL;
    size_t i;

    lwmsg_client_lock(client);

    while (!client->call_pool_available && client->call_pool_created == client->call_pool_capacity)
    {
        lwmsg_client_wait(client);
    }

    if (client->call_pool_available)
    {
        for (i = 0; i < client->call_pool_capacity; i++)
        {
            if (client->call_pool[i])
            {
                call = client->call_pool[i];
                client->call_pool[i] = NULL;
                client->call_pool_available--;
                break;
            }
        }
    }
    else if (client->call_pool_created < client->call_pool_capacity)
    {
        BAIL_ON_ERROR(status = lwmsg_client_call_new(client, &call));

        client->call_pool_created++;
    }

    BAIL_ON_ERROR(status = lwmsg_client_call_establish_assoc(call));

    *out_call = LWMSG_CALL(call);

done:

    lwmsg_client_unlock(client);

    return status;

error:

    if (call)
    {
        lwmsg_call_release(LWMSG_CALL(call));
    }

    goto done;
}

static
void
lwmsg_client_call_release(
    LWMsgCall* call
    )
{
    ClientCall* my_call = (ClientCall*) call;
    LWMsgClient* client = my_call->client;
    size_t i;

    if (my_call->destroy_assoc && my_call->assoc)
    {
        lwmsg_assoc_delete(my_call->assoc);
        my_call->assoc = NULL;
        my_call->destroy_assoc = LWMSG_FALSE;
    }

    lwmsg_client_lock(client);

    for (i = 0; i < client->call_pool_capacity; i++)
    {
        if (!client->call_pool[i])
        {
            client->call_pool[i] = my_call;
            client->call_pool_available++;

            lwmsg_client_signal(client);

            break;
        }
    }

    lwmsg_client_unlock(client);
}

LWMsgStatus
lwmsg_client_set_max_concurrent(
    LWMsgClient* client,
    size_t max
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    ClientCall** pool_new = NULL;
    size_t i;

    lwmsg_client_lock(client);

    /* If we are decreasing the number of connections */
    if (max < client->call_pool_capacity)
    {
        /* Close any idle connections in the slots we will be removing */
        for (i = max; i < client->call_pool_capacity; i++)
        {
            if (client->call_pool[i])
            {
                lwmsg_client_call_delete(client->call_pool[i]);
                client->call_pool[i] = NULL;
            }
        }
    }

    pool_new = realloc(client->call_pool, max * sizeof(*pool_new));

    if (!pool_new)
    {
        BAIL_ON_ERROR(status = LWMSG_STATUS_MEMORY);
    }

    /* If we are increasing the number of connections */
    if (max > client->call_pool_capacity)
    {
        /* Zero out any new slots and notify waiting threads that slots are available */
        memset(pool_new + client->call_pool_capacity, 0, (max - client->call_pool_capacity) * sizeof(*pool_new));
        lwmsg_client_signal(client);
    }

    client->call_pool = pool_new;
    client->call_pool_capacity = max;

error:

    lwmsg_client_unlock(client);

    return status;
}

LWMsgStatus
lwmsg_client_shutdown(
    LWMsgClient* client
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    size_t i;

    lwmsg_client_lock(client);

    while (client->call_pool_available < client->call_pool_created)
    {
        lwmsg_client_wait(client);
    }

    for (i = 0; i < client->call_pool_capacity; i++)
    {
        if (client->call_pool[i])
        {
            lwmsg_client_call_delete(client->call_pool[i]);
            client->call_pool[i] = NULL;
            client->call_pool_created--;
        }
    }

    lwmsg_client_unlock(client);

    return status;
}

void
lwmsg_client_delete(
    LWMsgClient* client
    )
{
    size_t i;

    for (i = 0; i < client->call_pool_capacity; i++)
    {
        if (client->call_pool[i])
        {
            lwmsg_client_call_delete(client->call_pool[i]);
        }
    }

    pthread_mutex_destroy(&client->lock);
    pthread_cond_destroy(&client->event);
    
    lwmsg_session_manager_delete(client->manager);

    free(client->endpoint);
    free(client->call_pool);
    free(client);
}

static
LWMsgStatus
lwmsg_client_call_dispatch(
    LWMsgCall* call,
    const LWMsgParams* in,
    LWMsgParams* out,
    LWMsgCompleteFunction complete,
    void* data
    )
{
    ClientCall* my_call = (ClientCall*) call;
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    LWMsgBool retry = LWMSG_FALSE;
    LWMsgMessage request = LWMSG_MESSAGE_INITIALIZER;
    LWMsgMessage response = LWMSG_MESSAGE_INITIALIZER;

    if (complete)
    {
        /* Async completion not yet supported */
        BAIL_ON_ERROR(status = LWMSG_STATUS_UNIMPLEMENTED);
    }

    request.tag = in->tag;
    request.data = in->data;

    do
    {
        status = lwmsg_assoc_send_message_transact(my_call->assoc, &request, &response);

        if (!retry &&
            (status == LWMSG_STATUS_PEER_RESET ||
             status == LWMSG_STATUS_PEER_CLOSE))
        {
            BAIL_ON_ERROR(status = lwmsg_assoc_reset(my_call->assoc));
            BAIL_ON_ERROR(status = lwmsg_assoc_establish(my_call->assoc));
            status = LWMSG_STATUS_AGAIN;
            retry = LWMSG_TRUE;
        }
    } while (status == LWMSG_STATUS_AGAIN);

    BAIL_ON_ERROR(status);
    BAIL_ON_ERROR(status = response.status);

    out->tag = response.tag;
    out->data = response.data;

done:

    return status;

error:

    my_call->destroy_assoc = LWMSG_TRUE;

    goto done;
}

static
LWMsgStatus
lwmsg_client_call_destroy_params(
    LWMsgCall* call,
    LWMsgParams* params
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    ClientCall* my_call = (ClientCall*) call;
    LWMsgMessage message = LWMSG_MESSAGE_INITIALIZER;

    message.tag = params->tag;
    message.data = params->data;

    BAIL_ON_ERROR(status = lwmsg_assoc_destroy_message(my_call->assoc, &message));

    params->tag = message.tag;
    params->data = message.data;

error:

    return status;
}

static
LWMsgSession*
lwmsg_client_call_get_session(
    LWMsgCall* call
    )
{
    ClientCall* my_call = (ClientCall*) call;
    LWMsgSession* session = NULL;
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;

    status = lwmsg_assoc_get_session(my_call->assoc, &session);
    ABORT_IF_FALSE(status == LWMSG_STATUS_SUCCESS);

    return session;
}

static LWMsgCallClass client_call_vtbl =
{
    .release = lwmsg_client_call_release,
    .dispatch = lwmsg_client_call_dispatch,
    .destroy_params = lwmsg_client_call_destroy_params,
    .get_session = lwmsg_client_call_get_session
};

LWMsgStatus
lwmsg_client_call_new(
    LWMsgClient* client,
    ClientCall** call
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    ClientCall* my_call = NULL;

    BAIL_ON_ERROR(status = LWMSG_ALLOC(&my_call));

    my_call->base.vtbl = &client_call_vtbl;
    my_call->client = client;

    BAIL_ON_ERROR(status = lwmsg_connection_new(client->context, client->protocol, &my_call->assoc));
    BAIL_ON_ERROR(status = lwmsg_assoc_set_session_manager(my_call->assoc, client->manager));
    BAIL_ON_ERROR(status = lwmsg_connection_set_endpoint(my_call->assoc, client->mode, client->endpoint));
    BAIL_ON_ERROR(status = lwmsg_assoc_establish(my_call->assoc));

    *call = my_call;

done:

    return status;

error:

    if (my_call)
    {
        lwmsg_client_call_delete(my_call);
    }

    goto done;
}

void
lwmsg_client_call_delete(
    ClientCall* call
    )
{
    if (call->assoc)
    {
        lwmsg_assoc_delete(call->assoc);
    }

    free(call);
}
