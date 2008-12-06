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
    LWMsgProtocol* protocol,
    LWMsgClient** out_client
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    LWMsgClient* client = NULL;

    client = calloc(1, sizeof(*client));

    lwmsg_context_setup(&client->context, &protocol->context);

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

    client->protocol = protocol;
    client->assoc_pool_capacity = 4;
    client->assoc_pool_created = 0;
    client->assoc_pool_available = 0;
    client->assoc_pool = calloc(client->assoc_pool_capacity, sizeof(*client->assoc_pool));

    if (!client->assoc_pool)
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

        if (client->assoc_pool)
        {
            free(client->assoc_pool);
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

    if (client->assoc_pool_created)
    {
        BAIL_ON_ERROR(status = LWMSG_STATUS_INVALID);
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

LWMsgStatus
lwmsg_client_acquire_assoc(
    LWMsgClient* client,
    LWMsgAssoc** out_assoc
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    LWMsgAssoc* assoc = NULL;
    size_t i;

    lwmsg_client_lock(client);

    while (!client->assoc_pool_available && client->assoc_pool_created == client->assoc_pool_capacity)
    {
        lwmsg_client_wait(client);
    }

    if (client->assoc_pool_available)
    {
        for (i = 0; i < client->assoc_pool_capacity; i++)
        {
            if (client->assoc_pool[i])
            {
                assoc = client->assoc_pool[i];
                client->assoc_pool[i] = 0;
                client->assoc_pool_available--;
                break;
            }
        }
    }
    else if (client->assoc_pool_created < client->assoc_pool_capacity)
    {
        BAIL_ON_ERROR(status = lwmsg_connection_new(client->protocol, &assoc));
        /* Force assoc context to route through us */
        assoc->context.parent = &client->context;

        BAIL_ON_ERROR(status = lwmsg_assoc_set_session_manager(assoc, client->manager));
        BAIL_ON_ERROR(status = lwmsg_connection_set_endpoint(assoc, client->mode, client->endpoint));

        client->assoc_pool_created++;
    }

    *out_assoc = assoc;

error:

    lwmsg_client_unlock(client);

    return status;
}

LWMsgStatus
lwmsg_client_create_assoc(
    LWMsgClient* client,
    LWMsgAssoc** out_assoc
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    LWMsgAssoc* assoc = NULL;

    BAIL_ON_ERROR(status = lwmsg_connection_new(client->protocol, &assoc));
    BAIL_ON_ERROR(status = lwmsg_assoc_set_session_manager(assoc, client->manager));
    BAIL_ON_ERROR(status = lwmsg_connection_set_endpoint(assoc, client->mode, client->endpoint));

    *out_assoc = assoc;

error:

    return status;
}
    

LWMsgStatus
lwmsg_client_release_assoc(
    LWMsgClient* client,
    LWMsgAssoc* assoc
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    size_t i;

    lwmsg_client_lock(client);

    if (!(client->assoc_pool_available < client->assoc_pool_created))
    {
        BAIL_ON_ERROR(status = LWMSG_STATUS_INVALID);
    }

    for (i = 0; i < client->assoc_pool_capacity; i++)
    {
        if (!client->assoc_pool[i])
        {
            client->assoc_pool[i] = assoc;
            client->assoc_pool_available++;
            assoc = NULL;

            lwmsg_client_signal(client);

            break;
        }
    }

error:

    lwmsg_client_unlock(client);

    if (assoc)
    {
        lwmsg_assoc_close(assoc);
        lwmsg_assoc_delete(assoc);
    }

    return status;
}

LWMsgStatus
lwmsg_client_send_message(
    LWMsgClient* client,
    LWMsgMessage* message
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    LWMsgAssoc* assoc = NULL;

    BAIL_ON_ERROR(status = lwmsg_client_acquire_assoc(client, &assoc));
    BAIL_ON_ERROR(status = lwmsg_assoc_send_message(assoc, message));

error:

    if (assoc)
    {
        lwmsg_client_release_assoc(client, assoc);
    }
    
    return status;
}

LWMsgStatus
lwmsg_client_recv_message(
    LWMsgClient* client,
    LWMsgMessage* message
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    LWMsgAssoc* assoc = NULL;

    BAIL_ON_ERROR(status = lwmsg_client_acquire_assoc(client, &assoc));
    BAIL_ON_ERROR(status = lwmsg_assoc_recv_message(assoc, message));

error:

    if (assoc)
    {
        lwmsg_client_release_assoc(client, assoc);
    }
    
    return status;
}

LWMsgStatus
lwmsg_client_send_message_transact(
    LWMsgClient* client,
    LWMsgMessage* send_msg,
    LWMsgMessage* recv_msg
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    LWMsgAssoc* assoc = NULL;

    BAIL_ON_ERROR(status = lwmsg_client_acquire_assoc(client, &assoc));
    BAIL_ON_ERROR(status = lwmsg_assoc_send_message_transact(assoc, send_msg, recv_msg));

error:

    if (assoc)
    {
        lwmsg_client_release_assoc(client, assoc);
    }
    
    return status;
}

LWMsgStatus
lwmsg_client_send(
    LWMsgClient* client,
    LWMsgMessageTag type,
    void* object
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    LWMsgAssoc* assoc = NULL;

    BAIL_ON_ERROR(status = lwmsg_client_acquire_assoc(client, &assoc));
    BAIL_ON_ERROR(status = lwmsg_assoc_send(assoc, type, object));

error:

    if (assoc)
    {
        lwmsg_client_release_assoc(client, assoc);
    }
    
    return status;
}

LWMsgStatus
lwmsg_client_recv(
    LWMsgClient* client,
    LWMsgMessageTag* out_type,
    void** out_object
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    LWMsgAssoc* assoc = NULL;

    BAIL_ON_ERROR(status = lwmsg_client_acquire_assoc(client, &assoc));
    BAIL_ON_ERROR(status = lwmsg_assoc_recv(assoc, out_type, out_object));

error:

    if (assoc)
    {
        lwmsg_client_release_assoc(client, assoc);
    }
    
    return status;
}

LWMsgStatus
lwmsg_client_send_transact(
    LWMsgClient* client,
    LWMsgMessageTag in_type,
    void* in_object,
    LWMsgMessageTag* out_type,
    void** out_object
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    LWMsgAssoc* assoc = NULL;

    BAIL_ON_ERROR(status = lwmsg_client_acquire_assoc(client, &assoc));
    BAIL_ON_ERROR(status = lwmsg_assoc_send_transact(assoc, in_type, in_object, out_type, out_object));

error:

    if (assoc)
    {
        lwmsg_client_release_assoc(client, assoc);
    }
    
    return status;
}

LWMsgStatus
lwmsg_client_set_max_concurrent(
    LWMsgClient* client,
    size_t max
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    LWMsgAssoc** pool_new = NULL;
    size_t i;

    lwmsg_client_lock(client);

    /* If we are decreasing the number of connections */
    if (max < client->assoc_pool_capacity)
    {
        /* Close any idle connections in the slots we will be removing */
        for (i = max; i < client->assoc_pool_capacity; i++)
        {
            if (client->assoc_pool[i])
            {
                BAIL_ON_ERROR(status = lwmsg_assoc_close(client->assoc_pool[i]));
                lwmsg_assoc_delete(client->assoc_pool[i]);
                client->assoc_pool[i] = NULL;
            }
        }
    }

    pool_new = realloc(client->assoc_pool, max * sizeof(*pool_new));

    if (!pool_new)
    {
        BAIL_ON_ERROR(status = LWMSG_STATUS_MEMORY);
    }

    /* If we are increasing the number of connections */
    if (max > client->assoc_pool_capacity)
    {
        /* Zero out any new slots and notify waiting threads that slots are available */
        memset(pool_new + client->assoc_pool_capacity, 0, (max - client->assoc_pool_capacity) * sizeof(*pool_new));
        lwmsg_client_signal(client);
    }

    client->assoc_pool = pool_new;
    client->assoc_pool_capacity = max;

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

    while (client->assoc_pool_available < client->assoc_pool_created)
    {
        lwmsg_client_wait(client);
    }

    for (i = 0; i < client->assoc_pool_capacity; i++)
    {
        if (client->assoc_pool[i])
        {
            BAIL_ON_ERROR(status = lwmsg_assoc_close(client->assoc_pool[i]));
            lwmsg_assoc_delete(client->assoc_pool[i]);
            client->assoc_pool[i] = NULL;
            client->assoc_pool_created--;
        }
    }

error:

    lwmsg_client_unlock(client);

    return status;
}

void
lwmsg_client_delete(
    LWMsgClient* client
    )
{
    size_t i;

    for (i = 0; i < client->assoc_pool_capacity; i++)
    {
        if (client->assoc_pool[i])
        {
            lwmsg_assoc_delete(client->assoc_pool[i]);
        }
    }

    pthread_mutex_destroy(&client->lock);
    pthread_cond_destroy(&client->event);
    
    lwmsg_session_manager_delete(client->manager);

    lwmsg_context_cleanup(&client->context);

    free(client->endpoint);
    free(client->assoc_pool);
    free(client);
}
