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
#include "assoc-private.h"
#include "util-private.h"
#include "protocol-private.h"
#include "session-private.h"

#define ACTION_ON_ERROR(_a_, _e_) do                            \
    {                                                           \
        LWMsgStatus __s__ = (_e_);                              \
        LWMsgAssoc* __a__ = (_a_);                              \
        switch (lwmsg_assoc_lookup_action(__a__, status))       \
        {                                                       \
        case LWMSG_ASSOC_ACTION_RETRY:                          \
            goto retry;                                         \
        case LWMSG_ASSOC_ACTION_RESET_AND_RETRY:                \
            BAIL_ON_ERROR(status = lwmsg_assoc_reset(__a__));   \
            goto retry;                                         \
        default:                                                \
            BAIL_ON_ERROR(__s__);                               \
        }                                                       \
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
    BAIL_ON_ERROR(status = assoc->aclass->get_session(assoc, assoc->timeout_set ? &assoc->timeout : NULL, &session));

    BAIL_ON_ERROR(status = lwmsg_session_manager_register_handle(
                      manager,
                      session,
                      typename,
                      handle,
                      free));

error:

    return status;
}

LWMsgStatus
lwmsg_assoc_unregister_handle(
    LWMsgAssoc* assoc,
    void* handle,
    LWMsgBool do_cleanup
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    LWMsgSessionManager* manager = NULL;
    LWMsgSession* session = NULL;

    BAIL_ON_ERROR(status = lwmsg_assoc_get_session_manager(assoc, &manager));
    BAIL_ON_ERROR(status = assoc->aclass->get_session(assoc, assoc->timeout_set ? &assoc->timeout : NULL, &session));

    BAIL_ON_ERROR(status = lwmsg_session_manager_unregister_handle(
                      manager,
                      session,
                      handle,
                      do_cleanup
                      ));
    
error:

    return status;
}

LWMsgStatus
lwmsg_assoc_get_handle_location(
    LWMsgAssoc* assoc,
    void* handle,
    LWMsgHandleLocation* location
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    LWMsgSessionManager* manager = NULL;
    LWMsgSession* session = NULL;

    BAIL_ON_ERROR(status = lwmsg_assoc_get_session_manager(assoc, &manager));
    BAIL_ON_ERROR(status = assoc->aclass->get_session(assoc, assoc->timeout_set ? &assoc->timeout : NULL, &session));

    BAIL_ON_ERROR(status = lwmsg_session_manager_handle_pointer_to_id(
                      manager,
                      session,
                      NULL,
                      handle,
                      LWMSG_FALSE,
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
    
    lwmsg_context_setup(&assoc->context, &prot->context);

    lwmsg_context_set_data_function(&assoc->context, lwmsg_assoc_context_get_data, assoc);

    if (aclass->construct)
    {
        aclass->construct(assoc);
    }

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
    switch (status)
    {
    case LWMSG_STATUS_TIMEOUT:
        return assoc->action_vector[LWMSG_ASSOC_EXCEPTION_TIMEOUT];
    case LWMSG_STATUS_EOF:
        switch (lwmsg_assoc_get_state(assoc))
        {
        case LWMSG_ASSOC_STATE_PEER_RESET:
            return assoc->action_vector[LWMSG_ASSOC_EXCEPTION_PEER_RESET];
        default:
            return LWMSG_ASSOC_ACTION_NONE;
        }
    default:
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
    
retry:

    ACTION_ON_ERROR(assoc, status = assoc->aclass->send_msg(assoc, message, assoc->timeout_set ? &assoc->timeout : NULL));
    
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
    
retry:
    
    ACTION_ON_ERROR(assoc, status = assoc->aclass->recv_msg(assoc, message, assoc->timeout_set ? &assoc->timeout : NULL));

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

retry:

    ACTION_ON_ERROR(assoc, status = assoc->aclass->send_msg(assoc, send_message, assoc->timeout_set ? &assoc->timeout : NULL));
    ACTION_ON_ERROR(assoc, status = assoc->aclass->recv_msg(assoc, recv_message, assoc->timeout_set ? &assoc->timeout : NULL));

error:

    return status;
}


LWMsgStatus
lwmsg_assoc_recv_message_transact(
    LWMsgAssoc* assoc,
    LWMsgDispatchFunction dispatch,
    void* data
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    LWMsgMessage recv_message = {-1, NULL};
    LWMsgMessage send_message = {-1, NULL};

retry:

    ACTION_ON_ERROR(assoc, status = assoc->aclass->recv_msg(assoc, &recv_message, assoc->timeout_set ? &assoc->timeout : NULL));
    ACTION_ON_ERROR(assoc, status = dispatch(assoc, &recv_message, &send_message, data));
    ACTION_ON_ERROR(assoc, status = assoc->aclass->send_msg(assoc, &send_message, assoc->timeout_set ? &assoc->timeout : NULL));
error:
    
    if (recv_message.object)
    {
        lwmsg_assoc_free_message(assoc, &recv_message);
    }

    if (send_message.object)
    {
        lwmsg_assoc_free_message(assoc, &send_message);
    }

    return status;
}


LWMsgStatus
lwmsg_assoc_send(
    LWMsgAssoc* assoc,
    LWMsgMessageTag type,
    void* object
    )
{
    LWMsgMessage message;

    message.tag = type;
    message.object = object;

    return lwmsg_assoc_send_message(assoc, &message);
}

LWMsgStatus
lwmsg_assoc_recv(
    LWMsgAssoc* assoc,
    LWMsgMessageTag* out_type,
    void** out_object
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    LWMsgMessage message;

    BAIL_ON_ERROR(status = lwmsg_assoc_recv_message(assoc, &message));

    *out_type = message.tag;
    *out_object = message.object;

error:

    return status;
}


LWMsgStatus
lwmsg_assoc_send_transact(
    LWMsgAssoc* assoc,
    LWMsgMessageTag in_type,
    void* in_object,
    LWMsgMessageTag* out_type,
    void** out_object
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    LWMsgMessage in_message;
    LWMsgMessage out_message;

    in_message.tag = in_type;
    in_message.object = in_object;

    BAIL_ON_ERROR(status = lwmsg_assoc_send_message_transact(assoc, &in_message, &out_message));

    *out_type = out_message.tag;
    *out_object = out_message.object;

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

retry:

    ACTION_ON_ERROR(assoc, status = assoc->aclass->get_peer_security_token(assoc, assoc->timeout_set ? &assoc->timeout : NULL, out_token));

error:

    return status;
}

LWMsgStatus
lwmsg_assoc_close(
    LWMsgAssoc* assoc
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;

    BAIL_ON_ERROR(status = assoc->aclass->close(assoc, assoc->timeout_set ? &assoc->timeout : NULL));

error:

    return status;
}

LWMsgStatus
lwmsg_assoc_reset(
    LWMsgAssoc* assoc
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;

    BAIL_ON_ERROR(status = assoc->aclass->reset(assoc, assoc->timeout_set ? &assoc->timeout : NULL));

error:

    return status;
}

LWMsgStatus
lwmsg_assoc_free_message(
    LWMsgAssoc* assoc,
    LWMsgMessage* message
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    LWMsgTypeSpec* type = NULL;

    BAIL_ON_ERROR(status = lwmsg_protocol_get_message_type(assoc->prot, message->tag, &type)); 
    BAIL_ON_ERROR(status = lwmsg_context_free_graph(&assoc->context, type, message->object));

error:

    return status;
}


LWMsgStatus
lwmsg_assoc_free_graph(
    LWMsgAssoc* assoc,
    LWMsgMessageTag mtype,
    void* object
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    LWMsgTypeSpec* type = NULL;

    BAIL_ON_ERROR(status = lwmsg_protocol_get_message_type(assoc->prot, mtype, &type)); 
    BAIL_ON_ERROR(status = lwmsg_context_free_graph(&assoc->context, type, object));

error:

    return status;
}

LWMsgStatus
lwmsg_assoc_set_timeout(
    LWMsgAssoc* assoc,
    LWMsgTime* timeout
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;

    if (timeout)
    {
        if (timeout->seconds < 0 || timeout->microseconds < 0)
        {
            BAIL_ON_ERROR(status = LWMSG_STATUS_INVALID);
        }

        assoc->timeout_set = 1;
        assoc->timeout = *timeout;
    }
    else
    {
        assoc->timeout_set = 0;
    }

error:

    return status;
}

void
lwmsg_assoc_set_timeout_ms(
    LWMsgAssoc* assoc,
    unsigned long ms
    )
{
    LWMsgTime timeout;

    timeout.seconds = ms / 1000;
    timeout.microseconds = (ms % 1000) * 1000;

    lwmsg_assoc_set_timeout(assoc, &timeout);
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
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;

    BAIL_ON_ERROR(status = assoc->aclass->get_state(assoc));

error:

    return status;
}

LWMsgStatus
lwmsg_assoc_set_action(
    LWMsgAssoc* assoc,
    LWMsgAssocException exception,
    LWMsgAssocAction action
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;

    
    if (exception >= LWMSG_ASSOC_EXCEPTION_COUNT ||
        action >= LWMSG_ASSOC_ACTION_COUNT)
    {
        ASSOC_RAISE_ERROR(assoc, status = LWMSG_STATUS_INVALID, "Invalid exception or action");
    }

    assoc->action_vector[exception] = action;

error:

    return status;
}

LWMsgStatus
lwmsg_assoc_set_session_data(
    LWMsgAssoc* assoc,
    void* data,
    LWMsgSessionDataCleanupFunction cleanup
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    LWMsgSessionManager* manager = NULL;
    LWMsgSession* session = NULL;

    /* In order to set the session data, we first need a session */
    BAIL_ON_ERROR(status = lwmsg_assoc_get_session_manager(assoc, &manager));
    BAIL_ON_ERROR(status = assoc->aclass->get_session(assoc, assoc->timeout_set ? &assoc->timeout : NULL, &session));

    BAIL_ON_ERROR(status = lwmsg_session_manager_set_session_data(
                      manager,
                      session,
                      data,
                      cleanup));

error:

    return status;
}

void*
lwmsg_assoc_get_session_data(
    LWMsgAssoc* assoc
    )
{
    LWMsgSession* session = NULL;

    if (!assoc->manager)
    {
        return NULL;
    }

    if (assoc->aclass->get_session(assoc, assoc->timeout_set ? &assoc->timeout : NULL, &session))
    {
        return NULL;
    }

    return lwmsg_session_manager_get_session_data(assoc->manager, session);
}
