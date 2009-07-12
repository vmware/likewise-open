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
 *        assoc.c
 *
 * Abstract:
 *
 *        Association API
 *        Primary entry points
 *
 * Authors: Brian Koropoff (bkoropoff@likewisesoftware.com)
 *
 */
#include <config.h>
#include <string.h>
#include <lwmsg/data.h>
#include "assoc-private.h"
#include "util-private.h"
#include "protocol-private.h"
#include "session-private.h"

#define MAX_RETRIES 1

#define ACTION_ON_ERROR(_a_, _e_) do                                    \
    {                                                                   \
        LWMsgStatus __s__ = (_e_);                                      \
        LWMsgAssoc* __a__ = (_a_);                                      \
        if (__s__)                                                      \
        {                                                               \
            switch (lwmsg_assoc_lookup_action(__a__, status))           \
            {                                                           \
            case LWMSG_ASSOC_ACTION_RETRY:                              \
                if (retries > MAX_RETRIES)                              \
                {                                                       \
                    BAIL_ON_ERROR(status = __s__);                      \
                }                                                       \
                else                                                    \
                {                                                       \
                    retries++;                                          \
                    goto retry;                                         \
                }                                                       \
            case LWMSG_ASSOC_ACTION_RESET_AND_RETRY:                    \
                if (retries > MAX_RETRIES)                              \
                {                                                       \
                    BAIL_ON_ERROR(status = __s__);                      \
                }                                                       \
                else                                                    \
                {                                                       \
                    BAIL_ON_ERROR(status = lwmsg_assoc_reset(__a__));   \
                    BAIL_ON_ERROR(status = lwmsg_assoc_establish(__a__)); \
                    retries++;                                          \
                    goto retry;                                         \
                }                                                       \
            default:                                                    \
                BAIL_ON_ERROR(status = __s__);                          \
            }                                                           \
        }                                                               \
    } while (0)

LWMsgStatus
lwmsg_assoc_register_handle(
    LWMsgAssoc* assoc,
    const char* typename,
    void* handle,
    LWMsgHandleCleanupFunction free
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    LWMsgSessionManager* manager = NULL;
    LWMsgSession* session = NULL;

    BAIL_ON_ERROR(status = lwmsg_assoc_get_session_manager(assoc, &manager));
    BAIL_ON_ERROR(status = assoc->aclass->get_session(assoc, &session));

    BAIL_ON_ERROR(status = lwmsg_session_manager_register_handle_local(
                      manager,
                      session,
                      typename,
                      handle,
                      free,
                      NULL));

error:

    return status;
}

LWMsgStatus
lwmsg_assoc_retain_handle(
    LWMsgAssoc* assoc,
    void* handle
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    LWMsgSessionManager* manager = NULL;
    LWMsgSession* session = NULL;

    BAIL_ON_ERROR(status = lwmsg_assoc_get_session_manager(assoc, &manager));
    BAIL_ON_ERROR(status = assoc->aclass->get_session(assoc, &session));

    BAIL_ON_ERROR(status = lwmsg_session_manager_retain_handle(
                      manager,
                      session,
                      handle
                      ));

error:

    return status;
}

LWMsgStatus
lwmsg_assoc_release_handle(
    LWMsgAssoc* assoc,
    void* handle
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    LWMsgSessionManager* manager = NULL;
    LWMsgSession* session = NULL;

    BAIL_ON_ERROR(status = lwmsg_assoc_get_session_manager(assoc, &manager));
    BAIL_ON_ERROR(status = assoc->aclass->get_session(assoc, &session));

    BAIL_ON_ERROR(status = lwmsg_session_manager_release_handle(
                      manager,
                      session,
                      handle
                      ));

error:

    return status;
}

LWMsgStatus
lwmsg_assoc_unregister_handle(
    LWMsgAssoc* assoc,
    void* handle
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    LWMsgSessionManager* manager = NULL;
    LWMsgSession* session = NULL;

    BAIL_ON_ERROR(status = lwmsg_assoc_get_session_manager(assoc, &manager));
    BAIL_ON_ERROR(status = assoc->aclass->get_session(assoc, &session));

    BAIL_ON_ERROR(status = lwmsg_session_manager_unregister_handle(
                      manager,
                      session,
                      handle
                      ));

error:

    return status;
}

LWMsgStatus
lwmsg_assoc_get_handle_location(
    LWMsgAssoc* assoc,
    void* handle,
    LWMsgHandleType* location
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    LWMsgSessionManager* manager = NULL;
    LWMsgSession* session = NULL;

    BAIL_ON_ERROR(status = lwmsg_assoc_get_session_manager(assoc, &manager));
    BAIL_ON_ERROR(status = assoc->aclass->get_session(assoc, &session));

    BAIL_ON_ERROR(status = lwmsg_session_manager_handle_pointer_to_id(
                      manager,
                      session,
                      handle,
                      NULL,
                      location,
                      NULL));

error:

    return status;
}

static LWMsgStatus
lwmsg_assoc_context_get_data(
    const char* key,
    void** out_data,
    void* data
    )
{
    if (!strcmp(key, "assoc"))
    {
        *out_data = data;
        return LWMSG_STATUS_SUCCESS;
    }
    else
    {
        return LWMSG_STATUS_NOT_FOUND;
    }
}

LWMsgStatus
lwmsg_assoc_new(
    const LWMsgContext* context,
    LWMsgProtocol* prot,
    LWMsgAssocClass* aclass,
    LWMsgAssoc** out_assoc
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;

    LWMsgAssoc* assoc = calloc(1, sizeof(LWMsgAssoc) + aclass->private_size);

    if (!assoc)
    {
        BAIL_ON_ERROR(status = LWMSG_STATUS_MEMORY);
    }

    assoc->prot = prot;
    assoc->aclass = aclass;

    lwmsg_context_setup(&assoc->context, context);

    lwmsg_context_set_data_function(&assoc->context, lwmsg_assoc_context_get_data, assoc);

    if (aclass->construct)
    {
        aclass->construct(assoc);
    }

    /* Default action vector */
    assoc->action_vector[LWMSG_STATUS_PEER_RESET] = LWMSG_ASSOC_ACTION_RESET_AND_RETRY;

    *out_assoc = assoc;

error:

    return status;
}

void
lwmsg_assoc_delete(
    LWMsgAssoc* assoc
    )
{
    lwmsg_context_cleanup(&assoc->context);
    if (assoc->aclass->destruct)
    {
        assoc->aclass->destruct(assoc);
    }
    
    if (assoc->manager && assoc->manager_is_private)
    {
        lwmsg_session_manager_delete(assoc->manager);
    }

    free(assoc);
}

void*
lwmsg_assoc_get_private(
    LWMsgAssoc* assoc
    )
{
    return (void*) assoc->private_data;
}

LWMsgProtocol*
lwmsg_assoc_get_protocol(
    LWMsgAssoc* assoc
    )
{
    return assoc->prot;
}

inline static
LWMsgAssocAction
lwmsg_assoc_lookup_action(
    LWMsgAssoc* assoc,
    LWMsgStatus status
    )
{
    if (status < (sizeof(assoc->action_vector) / sizeof(assoc->action_vector[0])))
    {
        return assoc->action_vector[status];
    }
    else
    {
        return LWMSG_ASSOC_ACTION_NONE;
    }
}

LWMsgStatus
lwmsg_assoc_send_message(
    LWMsgAssoc* assoc,
    LWMsgMessage* message
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    int retries = 0;

retry:

    ACTION_ON_ERROR(assoc, status = assoc->aclass->send_msg(assoc, message));
    
error:
    
    return status;
}

LWMsgStatus
lwmsg_assoc_recv_message(
    LWMsgAssoc* assoc,
    LWMsgMessage* message
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    int retries = 0;

retry:
    
    ACTION_ON_ERROR(assoc, status = assoc->aclass->recv_msg(assoc, message));

error:

    return status;
}

LWMsgStatus
lwmsg_assoc_send_message_transact(
    LWMsgAssoc* assoc,
    LWMsgMessage* send_message,
    LWMsgMessage* recv_message
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    int retries = 0;

retry:

    ACTION_ON_ERROR(assoc, status = assoc->aclass->send_msg(assoc, send_message));
    ACTION_ON_ERROR(assoc, status = assoc->aclass->recv_msg(assoc, recv_message));

error:

    return status;
}


LWMsgStatus
lwmsg_assoc_recv_message_transact(
    LWMsgAssoc* assoc,
    LWMsgAssocDispatchFunction dispatch,
    void* data
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    LWMsgMessage recv_message = LWMSG_MESSAGE_INITIALIZER;
    LWMsgMessage send_message = LWMSG_MESSAGE_INITIALIZER;
    int retries = 0;

retry:

    ACTION_ON_ERROR(assoc, status = assoc->aclass->recv_msg(assoc, &recv_message));
    ACTION_ON_ERROR(assoc, status = dispatch(assoc, &recv_message, &send_message, data));
    ACTION_ON_ERROR(assoc, status = assoc->aclass->send_msg(assoc, &send_message));

error:
    
    if (recv_message.tag != -1 && recv_message.data)
    {
        lwmsg_assoc_destroy_message(assoc, &recv_message);
    }

    if (send_message.tag != -1 && send_message.data)
    {
        lwmsg_assoc_destroy_message(assoc, &send_message);
    }

    return status;
}

LWMsgStatus
lwmsg_assoc_send(
    LWMsgAssoc* assoc,
    LWMsgTag type,
    void* object
    );

LWMsgStatus
lwmsg_assoc_recv(
    LWMsgAssoc* assoc,
    LWMsgTag* out_type,
    void** out_object
    );

LWMsgStatus
lwmsg_assoc_send_transact(
    LWMsgAssoc* assoc,
    LWMsgTag in_type,
    void* in_object,
    LWMsgTag* out_type,
    void** out_object
    );

LWMsgStatus
lwmsg_assoc_send(
    LWMsgAssoc* assoc,
    LWMsgTag type,
    void* object
    )
{
    LWMsgMessage message;

    message.tag = type;
    message.data = object;

    return lwmsg_assoc_send_message(assoc, &message);
}

LWMsgStatus
lwmsg_assoc_recv(
    LWMsgAssoc* assoc,
    LWMsgTag* out_type,
    void** out_object
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    LWMsgMessage message;

    BAIL_ON_ERROR(status = lwmsg_assoc_recv_message(assoc, &message));

    *out_type = message.tag;
    *out_object = message.data;

error:

    return status;
}


LWMsgStatus
lwmsg_assoc_send_transact(
    LWMsgAssoc* assoc,
    LWMsgTag in_type,
    void* in_object,
    LWMsgTag* out_type,
    void** out_object
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    LWMsgMessage in_message;
    LWMsgMessage out_message;

    in_message.tag = in_type;
    in_message.data = in_object;

    BAIL_ON_ERROR(status = lwmsg_assoc_send_message_transact(assoc, &in_message, &out_message));

    *out_type = out_message.tag;
    *out_object = out_message.data;

error:

    return status;
}

LWMsgStatus
lwmsg_assoc_get_peer_security_token(
    LWMsgAssoc* assoc,
    LWMsgSecurityToken** out_token
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;

    BAIL_ON_ERROR(status = assoc->aclass->get_peer_security_token(assoc, out_token));

error:

    return status;
}

LWMsgStatus
lwmsg_assoc_get_peer_session_id(
    LWMsgAssoc* assoc,
    LWMsgSessionID* id
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    LWMsgSession* session = NULL;
    const LWMsgSessionID* my_id = NULL;

    BAIL_ON_ERROR(status = assoc->aclass->get_session(assoc, &session));

    my_id = lwmsg_session_manager_get_session_id(assoc->manager, session);

    memcpy(id->bytes, my_id->bytes, sizeof(id->bytes));

error:

    return status;
}

LWMsgStatus
lwmsg_assoc_get_session(
    LWMsgAssoc* assoc,
    LWMsgSession** session
    )
{
    return assoc->aclass->get_session(assoc, session);
}

LWMsgStatus
lwmsg_assoc_close(
    LWMsgAssoc* assoc
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;

    BAIL_ON_ERROR(status = assoc->aclass->close(assoc));

error:

    return status;
}

LWMsgStatus
lwmsg_assoc_reset(
    LWMsgAssoc* assoc
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;

    BAIL_ON_ERROR(status = assoc->aclass->reset(assoc));

error:

    return status;
}

LWMsgStatus
lwmsg_assoc_destroy_message(
    LWMsgAssoc* assoc,
    LWMsgMessage* message
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    LWMsgTypeSpec* type = NULL;
    LWMsgDataContext* context = NULL;

    if (message->tag != -1)
    {
        BAIL_ON_ERROR(status = lwmsg_protocol_get_message_type(assoc->prot, message->tag, &type));

        if (type != NULL)
        {
            BAIL_ON_ERROR(status = lwmsg_data_context_new(&assoc->context, &context));
            BAIL_ON_ERROR(status = lwmsg_data_free_graph(context, type, message->data));
        }

        message->tag = -1;
        message->data = NULL;
    }

error:

    if (context)
    {
        lwmsg_data_context_delete(context);
    }

    return status;
}

LWMsgStatus
lwmsg_assoc_free_graph(
    LWMsgAssoc* assoc,
    LWMsgTag mtype,
    void* object
    );


LWMsgStatus
lwmsg_assoc_free_graph(
    LWMsgAssoc* assoc,
    LWMsgTag mtype,
    void* object
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    LWMsgTypeSpec* type = NULL;
    LWMsgDataContext* context = NULL;

    BAIL_ON_ERROR(status = lwmsg_protocol_get_message_type(assoc->prot, mtype, &type));
    BAIL_ON_ERROR(status = lwmsg_data_context_new(&assoc->context, &context));
    BAIL_ON_ERROR(status = lwmsg_data_free_graph(context, type, object));

error:

    if (context)
    {
        lwmsg_data_context_delete(context);
    }

    return status;
}

const char*
lwmsg_assoc_get_error_message(
    LWMsgAssoc* assoc,
    LWMsgStatus status
    )
{
    return lwmsg_context_get_error_message(&assoc->context, status);
}

LWMsgStatus
lwmsg_assoc_set_session_manager(
    LWMsgAssoc* assoc,
    LWMsgSessionManager* manager
    )
{
    if (assoc->manager && assoc->manager_is_private)
    {
        lwmsg_session_manager_delete(assoc->manager);
        assoc->manager_is_private = LWMSG_FALSE;
    }
    
    assoc->manager = manager;

    return LWMSG_STATUS_SUCCESS;
}

LWMsgStatus
lwmsg_assoc_get_session_manager(
    LWMsgAssoc* assoc,
    LWMsgSessionManager** out_manager
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;

    if (!assoc->manager)
    {
        BAIL_ON_ERROR(status = lwmsg_default_session_manager_new(&assoc->manager));
        assoc->manager_is_private = LWMSG_TRUE;
    }

    *out_manager = assoc->manager;

error:

    return status;
}

LWMsgAssocState
lwmsg_assoc_get_state(
    LWMsgAssoc* assoc
    )
{
    return assoc->aclass->get_state(assoc);
}

LWMsgStatus
lwmsg_assoc_set_action(
    LWMsgAssoc* assoc,
    LWMsgStatus condition,
    LWMsgAssocAction action
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;

    
    if (condition >= LWMSG_STATUS_COUNT || action >= LWMSG_ASSOC_ACTION_COUNT)
    {
        ASSOC_RAISE_ERROR(assoc, status = LWMSG_STATUS_INVALID_PARAMETER,
                          "Invalid status or action");
    }

    assoc->action_vector[condition] = action;

error:

    return status;
}

LWMsgStatus
lwmsg_assoc_get_session_data(
    LWMsgAssoc* assoc,
    void** data
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    LWMsgSession* session = NULL;

    if (!assoc->manager)
    {
        BAIL_ON_ERROR(status = LWMSG_STATUS_INVALID_STATE);
    }

    if (assoc->aclass->get_session(assoc, &session))
    {
        BAIL_ON_ERROR(status = LWMSG_STATUS_INVALID_STATE);
    }

    *data = lwmsg_session_manager_get_session_data(assoc->manager, session);

error:

    return status;
}

LWMsgStatus
lwmsg_assoc_set_timeout(
    LWMsgAssoc* assoc,
    LWMsgTimeout type,
    LWMsgTime* value
    )
{
    return assoc->aclass->set_timeout(assoc, type, value);
}

LWMsgStatus
lwmsg_assoc_establish(
    LWMsgAssoc* assoc
    )
{
    return assoc->aclass->establish(
        assoc,
        assoc->construct,
        assoc->destruct,
        assoc->construct_data);
}

LWMsgStatus
lwmsg_assoc_finish(
    LWMsgAssoc* assoc
    )
{
    return assoc->aclass->finish(assoc);
}

LWMsgStatus
lwmsg_assoc_set_nonblock(
    LWMsgAssoc* assoc,
    LWMsgBool nonblock
    )
{
    return assoc->aclass->set_nonblock(assoc, nonblock);
}

LWMsgStatus
lwmsg_assoc_set_session_functions(
    LWMsgAssoc* assoc,
    LWMsgSessionConstructor construct,
    LWMsgSessionDestructor destruct,
    void* data
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;

    assoc->construct = construct;
    assoc->destruct = destruct;
    assoc->construct_data = data;

    return status;
}

LWMsgStatus
lwmsg_assoc_print_message_alloc(
    LWMsgAssoc* assoc,
    LWMsgMessage* message,
    char** result
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    LWMsgDataContext* context = NULL;
    LWMsgTypeSpec* type = NULL;
    char* payload_result = NULL;
    char* my_result = NULL;
    const char* tag_name = NULL;

    BAIL_ON_ERROR(status = lwmsg_data_context_new(&assoc->context, &context));
    BAIL_ON_ERROR(status = lwmsg_protocol_get_message_name(assoc->prot, message->tag, &tag_name));
    BAIL_ON_ERROR(status = lwmsg_protocol_get_message_type(assoc->prot, message->tag, &type));

    if (type)
    {
        BAIL_ON_ERROR(status = lwmsg_data_print_graph_alloc(context, type, message->data, &payload_result));

        my_result = lwmsg_format("%s: %s", tag_name, payload_result);
    }
    else
    {
        my_result = lwmsg_format("%s: <null>", tag_name);
    }

    if (!my_result)
    {
        BAIL_ON_ERROR(status = LWMSG_STATUS_MEMORY);
    }

    *result = my_result;

cleanup:

    if (context)
    {
        lwmsg_data_context_delete(context);
    }

    if (payload_result)
    {
        lwmsg_context_free(&assoc->context, payload_result);
    }

    return status;

error:

    *result = NULL;

    if (my_result)
    {
        lwmsg_context_free(&assoc->context, my_result);
    }

    goto cleanup;
}
