#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <signal.h>
#include <sys/stat.h>
#include <pthread.h>

#include "fserver.h"
#include "protocol.h"

static void
blocked_signal_set(sigset_t* set)
{
    sigemptyset(set);
    sigaddset(set, SIGTERM);
    sigaddset(set, SIGINT);
    sigaddset(set, SIGPIPE);
}

static void
block_signals(void)
{
    sigset_t blocked;

    blocked_signal_set(&blocked);

    pthread_sigmask(SIG_BLOCK, &blocked, NULL);
}

static int
wait_signal(void)
{
    sigset_t wait;
    int sig = 0;

    blocked_signal_set(&wait);

    sigwait(&wait, &sig);

    return sig;
}

static int
run(LWMsgServer* server)
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    int ret = 0;
    int done = 0;
    int sig = 0;

    block_signals();

    status = lwmsg_server_start(server);
    if (status)
    {
        goto error;
    }

    while (!done)
    {
        sig = wait_signal();

        switch (sig)
        {
        case SIGINT:
        case SIGTERM:
            done = 1;
            break;
        default:
            break;
        }
    }

    status = lwmsg_server_stop(server);
    if (status)
    {
        goto error;
    }
    
error:

    if (status != LWMSG_STATUS_SUCCESS)
    {
        ret = -1;
    }

    return ret;
}

int main(int argc, char** argv)
{
    int ret = 0;
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    LWMsgProtocol* protocol = NULL;
    LWMsgServer* server = NULL;

    status = lwmsg_protocol_new(NULL, &protocol);
    if (status)
    {
        goto error;
    }

    status = lwmsg_protocol_add_protocol_spec(protocol, fserv_get_protocol());
    if (status)
    {
        goto error;
    }

    status = lwmsg_server_new(protocol, &server);
    if (status)
    {
        goto error;
    }

    status = lwmsg_server_add_dispatch_spec(server, fserver_get_dispatch());
    if (status)
    {
        goto error;
    }

    status = lwmsg_server_set_endpoint(server, LWMSG_SERVER_MODE_LOCAL, FSERV_SOCKET_PATH, (S_IRWXU | S_IRWXG | S_IRWXO));
    if (status)
    {
        goto error;
    }

    ret = run(server);

error:

    lwmsg_server_delete(server);

    if (status != LWMSG_STATUS_SUCCESS)
    {
        ret = -1;
    }

    exit(ret ? 1 : 0);
}

