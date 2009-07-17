#include "fserv-private.h"

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

static LWMsgProtocol* protocol = NULL;

/* Connect to local fserv */
int
fserv_connect(
    FServ** out_fserv
    )
{
    int ret = 0;
    FServ* fserv = NULL;
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;

    if (!protocol)
    {
        /* Create protocol object */
        status = lwmsg_protocol_new(NULL, &protocol);
        if (status)
        {
            ret = -1;
            goto error;
        }
        /* Add protocol spec to protocol object */
        status = lwmsg_protocol_add_protocol_spec(protocol, fserv_get_protocol());
        if (status)
        {
            ret = -1;
            goto error;
        }
    }

    fserv = calloc(1, sizeof(*fserv));
    if (!fserv)
    {
        ret = ENOMEM;
        goto error;
    }

    /* Create connection object */
    status = lwmsg_connection_new(NULL, protocol, &fserv->assoc);
    if (status)
    {
        ret = -1;
        goto error;
    }

    /* Set connection endpoint */
    status = lwmsg_connection_set_endpoint(
        fserv->assoc,
        LWMSG_CONNECTION_MODE_LOCAL,
        FSERV_SOCKET_PATH);
    if (status)
    {
        ret = -1;
        goto error;
    }

    /* Establish session */
    status = lwmsg_assoc_establish(fserv->assoc);
    if (status)
    {
        ret = -1;
        goto error;
    }

    *out_fserv = fserv;

done:

    return ret;

error:

    if (fserv)
    {
        if (fserv->assoc)
        {
            lwmsg_assoc_delete(fserv->assoc);
        }
        free(fserv);
    }

    goto done;
}

/* Disconnect an fserv connection */
int
fserv_disconnect(
    FServ* fserv
    )
{
    int ret = 0;
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;

    /* Close association */
    status = lwmsg_assoc_close(fserv->assoc);

    if (status)
    {
        ret = -1;
        goto error;
    }

error:

    /* Delete association */
    lwmsg_assoc_delete(fserv->assoc);
    free(fserv);

    return ret;
}

/* Open a file using an fserv connection */
int
fserv_open(
    FServ* fserv,
    const char* path,
    FServMode mode,
    FServFile** out_file
    )
{
    int ret = 0;
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    OpenRequest request;
    LWMsgMessage request_msg = LWMSG_MESSAGE_INITIALIZER;
    LWMsgMessage reply_msg = LWMSG_MESSAGE_INITIALIZER;
    FServFile* file = NULL;

    /* Set up request parameters */
    request.mode = mode;
    request.path = (char*) path;   
    request_msg.tag = FSERV_OPEN;
    request_msg.data = &request;
    
    /* Send message and receive reply */
    status = lwmsg_assoc_send_message_transact(fserv->assoc, &request_msg, &reply_msg);
    if (status)
    {
        ret = -1;
        goto error;
    }

    switch (reply_msg.tag)
    {
    case FSERV_OPEN_SUCCESS:
        /* Open succeeded -- create the client handle */
        file = malloc(sizeof(*file));
        if (!file)
        {
            ret = ENOMEM;
            goto error;
        }

        file->fserv = fserv;
        file->handle = (FileHandle*) reply_msg.data;
        reply_msg.data = NULL;
        *out_file = file;
        break;
    case FSERV_OPEN_FAILED:
        /* Open failed -- extract the error code */
        ret = ((StatusReply*) reply_msg.data)->err;
        goto error;
    default:
        ret = EINVAL;
        goto error;
    }

done:

    /* Ask the association to free the reply */
    lwmsg_assoc_destroy_message(fserv->assoc, &reply_msg);

    return ret;

error:

    if (file)
    {
        free(file);
    }

    goto done;
}

/* Read from a file */
int
fserv_read(
    FServFile* file,
    unsigned long size,
    void* buffer,
    unsigned long* size_read
    )
{
    int ret = 0;
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    ReadRequest request;
    ReadReply* reply;
    LWMsgMessage request_msg = LWMSG_MESSAGE_INITIALIZER;
    LWMsgMessage reply_msg = LWMSG_MESSAGE_INITIALIZER;

    /* Set up request parameters */
    request.handle = file->handle;
    request.size = size;
    request_msg.tag = FSERV_READ;
    request_msg.data = &request;
    
    /* Send message and receive reply */
    status = lwmsg_assoc_send_message_transact(file->fserv->assoc, &request_msg, &reply_msg);
    if (status)
    {
        ret = -1;
        goto error;
    }

    switch (reply_msg.tag)
    {
    case FSERV_READ_SUCCESS:
        /* Read succeeded -- copy the data into the buffer */
        reply = (ReadReply*) reply_msg.data;
        memcpy(buffer, reply->data, reply->size);
        *size_read = reply->size;
        break;
    case FSERV_READ_FAILED:
        /* Read failed -- extract the error code */
        ret = ((StatusReply*) reply_msg.data)->err;
        goto error;
    default:
        ret = EINVAL;
        goto error;
    }

done:

    /* Ask the association to free the reply */
    lwmsg_assoc_destroy_message(file->fserv->assoc, &reply_msg);

    return ret;

error:
    
    goto done;
}

/* Write to a file */
int
fserv_write(
    FServFile* file,
    unsigned long size,
    void* buffer
    )
{
    int ret = 0;
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    WriteRequest request;
    LWMsgMessage request_msg = LWMSG_MESSAGE_INITIALIZER;
    LWMsgMessage reply_msg = LWMSG_MESSAGE_INITIALIZER;

    /* Set up request parameters */
    request.handle = file->handle;   
    request.size = size;
    request.data = (char*) buffer;
    request_msg.tag = FSERV_WRITE;
    request_msg.data = &request;
    
    /* Send message and receive reply */
    status = lwmsg_assoc_send_message_transact(file->fserv->assoc, &request_msg, &reply_msg);
    if (status)
    {
        ret = -1;
        goto error;
    }

    switch (reply_msg.tag)
    {
    case FSERV_WRITE_SUCCESS:
        /* Write succeeded -- don't bother to look at reply message */
        break;
    case FSERV_WRITE_FAILED:
        /* Write failed -- extract the error code */
        ret = ((StatusReply*) reply_msg.data)->err;
        goto error;
    default:
        ret = EINVAL;
        goto error;
    }

done:

    /* Ask the association to free the reply */
    lwmsg_assoc_destroy_message(file->fserv->assoc, &reply_msg);

    return ret;

error:

    goto done;
}

/* Close a file */
int
fserv_close(
    FServFile* file
    )
{
    int ret = 0;
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    LWMsgMessage request_msg = LWMSG_MESSAGE_INITIALIZER;
    LWMsgMessage reply_msg = LWMSG_MESSAGE_INITIALIZER;

    request_msg.tag = FSERV_CLOSE;
    request_msg.data = file->handle;

    /* Send message and receive reply */
    status = lwmsg_assoc_send_message_transact(file->fserv->assoc, &request_msg, &reply_msg);
    if (status)
    {
        ret = -1;
        goto error;
    }

    switch (reply_msg.tag)
    {
    case FSERV_CLOSE_SUCCESS:
        /* Release the handle */
        status = lwmsg_assoc_release_handle(file->fserv->assoc, file->handle);
        if (status)
        {
            ret = -1;
            goto error;
        }
        file->handle = NULL;
        break;
    case FSERV_CLOSE_FAILED:
        /* In either case, extract the status code and get out of here */
        ret = ((StatusReply*) reply_msg.data)->err;
        if (ret)
        {
            goto error;
        }
        break;
    default:
        ret = EINVAL;
        goto error;
    }

error:

    /* Ask the association to free the reply */
    lwmsg_assoc_destroy_message(file->fserv->assoc, &reply_msg);

    /* Always free the file, even if there was a problem */
    free(file);

    return ret;
}
