#include "protocol-server.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>

#define LOG(...) fprintf(stderr, __VA_ARGS__)

int
fserv_check_permissions(LWMsgAssoc* assoc, const char* path, OpenMode mode)
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    int ret = 0;
    LWMsgSecurityToken* token = NULL;
    uid_t euid;
    gid_t egid;
    struct stat statbuf;

    status = lwmsg_assoc_get_peer_security_token(assoc, &token);
    if (status)
    {
        LOG("Failed to get auth info for association %p\n", assoc);
        ret = -1;
        goto error;
    }

    if (token == NULL || strcmp(lwmsg_security_token_get_type(token), "local"))
    {
        LOG("Unsupported authentication type on association %p\n", assoc);
        ret = -1;
        goto error;
    }

    status = lwmsg_local_token_get_eid(token, &euid, &egid);
    if (status)
    {
        ret = -1;
        goto error;
    }

    if (stat(path, &statbuf) == -1)
    {
        ret = errno;
        goto error;
    }

    if ((mode & OPEN_MODE_READ))
    {
        int can_read =
            ((statbuf.st_uid == euid && statbuf.st_mode & S_IRUSR) ||
             (statbuf.st_gid == egid && statbuf.st_mode & S_IRGRP) ||
             (statbuf.st_mode & S_IROTH));

        if (!can_read)
        {
            LOG("Permission denied for (uid = %i, gid = %i) to read %s\n",
                (int) euid,
                (int) egid,
                path);
            ret = EPERM;
            goto error;
        }
    }

    if ((mode & OPEN_MODE_WRITE))
    {
        int can_write =
            ((statbuf.st_uid == euid && statbuf.st_mode & S_IWUSR) ||
             (statbuf.st_gid == egid && statbuf.st_mode & S_IWGRP) ||
             (statbuf.st_mode & S_IWOTH));

        if (!can_write)
        {
            LOG("Permission denied for (uid = %i, gid = %i) to write %s\n",
                (int) euid,
                (int) egid,
                path);
            ret = EPERM;
            goto error;
        }
    }

    LOG("Permission granted for (uid = %i, gid = %i) to open %s\n",
        (int) euid,
        (int) egid,
        path);

error:
    
    return ret;
}

static LWMsgStatus
fserv_open_srv(
    LWMsgAssoc* assoc,
    const LWMsgMessage* in,
    LWMsgMessage* out,
    void* data
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    OpenRequest* req = (OpenRequest*) in->object;
    FileHandle* handle = NULL;
    StatusReply* sreply = NULL;
    int flags = 0;
    int fd = -1;
    int ret;
    
    LOG("fserv_open() of %s on association %p\n", req->path, assoc);

    ret = fserv_check_permissions(assoc, req->path, req->mode);

    if (ret)
    {
        sreply = malloc(sizeof(*sreply));
        
        if (!handle)
        {
            status = LWMSG_STATUS_MEMORY;
            goto error;
        }
        
        sreply->err = ret;
        out->tag = FSERV_OPEN_FAILED;
        out->object = (void*) sreply;
    }
    else
    {
        if ((req->mode & OPEN_MODE_READ) && !(req->mode & OPEN_MODE_WRITE))
        {
            flags |= O_RDONLY;
        }
        
        if ((req->mode & OPEN_MODE_WRITE) && !(req->mode & OPEN_MODE_READ))
        {
            flags |= O_WRONLY;
        }
        
        if ((req->mode & OPEN_MODE_WRITE) && (req->mode & OPEN_MODE_READ))
        {
            flags |= O_RDWR;
        }
        
        if ((req->mode & OPEN_MODE_APPEND))
        {
            flags |= O_APPEND;
        }
        
        fd = open(req->path, flags);
        
        if (fd >= 0)
        {
            handle = malloc(sizeof(*handle));
            
            if (!handle)
            {
                status = LWMSG_STATUS_MEMORY;
                goto error;
            }
            
            handle->fd = fd;
            handle->mode = req->mode;
            
            out->tag = FSERV_OPEN_SUCCESS;
            out->object = (void*) handle;

            LOG("Successfully opened %s as fd %i for association %p\n",
                req->path, fd, assoc);

            lwmsg_assoc_register_handle(assoc, "FileHandle", handle, free);
        }
        else
        {
            sreply = malloc(sizeof(*sreply));
            
            if (!handle)
            {
                status = LWMSG_STATUS_MEMORY;
                goto error;
            }
            
            sreply->err = errno;
            out->tag = FSERV_OPEN_FAILED;
            out->object = (void*) sreply;
        }
    }

error:

    return status;
}

static LWMsgStatus
fserv_write_srv(
    LWMsgAssoc* assoc,
    const LWMsgMessage* in,
    LWMsgMessage* out,
    void* data
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    WriteRequest* req = (WriteRequest*) in->object;
    StatusReply* sreply = NULL;
    int fd = req->handle->fd;
    
    LOG("fserv_write() of %lu bytes to fd %i on association %p\n",
        (unsigned long) req->size, fd, assoc);

    sreply = malloc(sizeof(*sreply));

    if (!sreply)
    {
        status = LWMSG_STATUS_MEMORY;
        goto error;
    }

    if (write(fd, req->data, req->size) == -1)
    {
        sreply->err = errno;
        out->tag = FSERV_WRITE_SUCCESS;
    }
    else
    {
        sreply->err = 0;
        out->tag = FSERV_WRITE_FAILED;
    }

    out->object = (void*) sreply;

error:
    
    return status;
}

static LWMsgStatus
fserv_read_srv(
    LWMsgAssoc* assoc,
    const LWMsgMessage* in,
    LWMsgMessage* out,
    void* data
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    ReadRequest* req = (ReadRequest*) in->object;
    StatusReply* sreply = NULL;
    ReadReply* rreply = NULL;
    int fd = req->handle->fd;
    int ret = 0;
    
    LOG("fserv_read() of %lu bytes from fd %i on association %p\n",
        (unsigned long) req->size, fd, assoc);

    rreply = malloc(sizeof(*rreply));

    if (!rreply)
    {
        status = LWMSG_STATUS_MEMORY;
        goto error;
    }

    rreply->data = malloc(req->size);
    
    if (!rreply->data)
    {
        status = LWMSG_STATUS_MEMORY;
        goto error;
    }

    ret = read(fd, rreply->data, req->size);

    if (ret == -1)
    {
        int err = errno;

        free(rreply->data);
        free(rreply);

        sreply = malloc(sizeof(*sreply));

        if (!sreply)
        {
            status = LWMSG_STATUS_MEMORY;
            goto error;
        }

        sreply->err = err;
        out->tag = FSERV_READ_FAILED;
        out->object = (void*) sreply;
    }
    else if (ret == 0)
    {
        free(rreply->data);
        rreply->data = NULL;
        rreply->size = 0;
        out->tag = FSERV_READ_SUCCESS;
        out->object = (void*) rreply;
    }
    else
    {
        rreply->size = ret;
        out->tag = FSERV_READ_SUCCESS;
        out->object = (void*) rreply;
    }

error:
    
    return status;
}

static LWMsgStatus
fserv_close_srv(
    LWMsgAssoc* assoc,
    const LWMsgMessage* in,
    LWMsgMessage* out,
    void* data
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    FileHandle* handle = (FileHandle*) in->object;
    StatusReply* sreply = NULL;

    LOG("fserv_close() on fd %i for association %p\n", handle->fd, assoc);

    sreply = malloc(sizeof(*sreply));
    
    if (!sreply)
    {
        status = LWMSG_STATUS_MEMORY;
        goto error;
    }
    
    if (close(handle->fd) == -1)
    {
        sreply->err = errno;
        out->tag = FSERV_CLOSE_FAILED;
        out->object = sreply;
    }
    else
    {
        sreply->err = 0;
        out->tag = FSERV_CLOSE_SUCCESS;
        out->object = sreply;       
        lwmsg_assoc_unregister_handle(assoc, handle, LWMSG_TRUE);
    }

error:

    return status;
}

static LWMsgDispatchSpec fserv_dispatch[] =
{
    LWMSG_DISPATCH(FSERV_OPEN, fserv_open_srv),
    LWMSG_DISPATCH(FSERV_WRITE, fserv_write_srv),
    LWMSG_DISPATCH(FSERV_READ, fserv_read_srv),
    LWMSG_DISPATCH(FSERV_CLOSE, fserv_close_srv),
    LWMSG_DISPATCH_END
};

LWMsgDispatchSpec*
fserver_get_dispatch(void)
{
    return fserv_dispatch;
}
