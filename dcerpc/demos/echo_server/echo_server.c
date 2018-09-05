/*
 * echo_server      : demo DCE RPC application
 *
 * Jim Doyle, jrd@bu.edu  09-05-1998
 *
 *
 */
#if HAVE_CONFIG_H
#include <config.h>
#endif

#ifndef _POSIX_PTHREAD_SEMANTICS
#define _POSIX_PTHREAD_SEMANTICS 1
#endif

#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <stdlib.h>
#include <lw/base.h>

#include <dce/dcethread.h>
#include <dce/dce_error.h>
#define RPC_FIELD_COUNT(x) ((x)->count)
#define RPC_FIELD_BINDING_H(x) ((x)->binding_h)

#define TRANSPORT_SIZE 2048

#include "echo.h"
#include "misc.h"

#ifndef HAVE_GETOPT_H
#include "getopt.h"
#endif

#ifndef _WIN32
#include <unistd.h>
static void wait_for_signals();
void sigpipe_handler(void);
#else /* _WIN32 */
#include <process.h>
#endif

extern int FIPS_mode_set(int);

/*
 *
 * A template DCE RPC server
 *
 * main() contains the basic calls needed to register an interface,
 * get communications endpoints, and register the endpoints
 * with the endpoint mapper.
 *
 * ReverseIt() implements the interface specified in echo.idl
 *
 */

static void
bind_server(
    rpc_binding_vector_p_t * server_binding,
    rpc_if_handle_t interface_spec,
    char * protocol,
    char * endpoint
    )
{
    char * function = "n/a";
    unsigned32 status;

    /*
     * Prepare the server binding handle
     * use all avail protocols (UDP and TCP). This basically allocates
     * new sockets for us and associates the interface UUID and
     * object UUID of with those communications endpoints.
     */

#if 0
    rpc_server_use_all_protseqs_if(0, interface_spec, &status);
#else
    if (!endpoint)
    {
        if (!protocol)
        {
            function = "rpc_server_use_all_protseqs()";
            rpc_server_use_all_protseqs(rpc_c_protseq_max_calls_default, &status);
        }
        else
        {
            function = "rpc_server_use_protseq()";
            rpc_server_use_protseq((unsigned char *) protocol, rpc_c_protseq_max_calls_default, &status);
        }
    }
    else
    {
        function = "rpc_server_use_protseq_ep()";
        rpc_server_use_protseq_ep((unsigned char *) protocol,
                            rpc_c_protseq_max_calls_default,
                            (unsigned char *) endpoint,
                            &status);
    }
#endif

    chk_dce_err(status, function, "", 1);
    rpc_server_inq_bindings(server_binding, &status);
    chk_dce_err(status, "rpc_server_inq_bindings()", "", 1);
}

static void usage()
{
    printf("usage: echo_server [-a name] [-e endpoint] [-n] [-u] [-t]\n");
    printf("         -a:  specify authentication identity\n");
    printf("         -e:  specify endpoint\n");
    printf("         -l:  use ncalrcp protocol\n");
    printf("         -n:  use named pipe protocol\n");
    printf("         -u:  use UDP protocol\n");
    printf("         -t:  use TCP protocol (default)\n");
    printf("\n");
    exit(1);
}


int main(int argc, char *argv[])
{
    unsigned32 status = 0;
    rpc_binding_vector_p_t server_binding = NULL;
    unsigned char *string_binding = NULL;
    unsigned32 i = 0;
    char * protocol[5] = {0};
    char * endpoint = "31415";
    int c;
    int protocol_idx = 0;
    char * spn = NULL;
    char *envptr = NULL;

    envptr = getenv("FIPS_MODE_SET");
    if (envptr)
    {
        FIPS_mode_set(atoi(envptr));
    }
    else
    {
        FIPS_mode_set(1);
    }

    /*
     * Process the cmd line args
     */
    while ((c = getopt(argc, argv, "a:e:nutl")) != EOF)
    {
        switch (c)
        {
        case 'a':
            spn = optarg;
            break;
        case 'e':
            endpoint = optarg;
            break;
        case 'l':
            if (protocol_idx < sizeof(protocol)/sizeof(protocol[0]))
            {
                protocol[protocol_idx++] = PROTOCOL_NCALRPC;
            }
            break;
        case 'n':
            if (protocol_idx < sizeof(protocol)/sizeof(protocol[0]))
            {
                protocol[protocol_idx++] = PROTOCOL_NP;
            }
            break;
        case 'u':
            if (protocol_idx < sizeof(protocol)/sizeof(protocol[0]))
            {
                protocol[protocol_idx++] = PROTOCOL_UDP;
            }
            break;
        case 't':
            if (protocol_idx < sizeof(protocol)/sizeof(protocol[0]))
            {
                protocol[protocol_idx++] = PROTOCOL_TCP;
            }
            break;
        default:
            usage();
        }
    }

    if (endpoint && !protocol[0])
    {
        protocol[protocol_idx++] = PROTOCOL_TCP;
        printf("NOTICE: Default protocol is '%s'\n", PROTOCOL_TCP);
    }

    if (spn)
    {
        rpc_server_register_auth_info(
            (unsigned char *) spn,
            rpc_c_authn_gss_negotiate,
            NULL,
            NULL,
            &status);
        if (status)
        {
            printf ("Couldn't set auth info. exiting.\n");
            return(1);
        }
    }

    /*
     * Register the Interface with the local endpoint mapper (rpcd)
     */

    printf ("Registering server.... \n");
    rpc_server_register_if(echo_v1_0_s_ifspec,
                           NULL,
                           NULL,
                           &status);
    chk_dce_err(status, "rpc_server_register_if()", "", 1);

    printf("registered.\nPreparing binding handle...\n");

    for (i=0; i<protocol_idx; i++)
    {
        if (server_binding)
        {
            rpc_binding_vector_free(&server_binding, &status);
        }
        bind_server(&server_binding,
                    echo_v1_0_s_ifspec,
                    protocol[i],
                    endpoint);
    }

    /*
     * Register bindings with the endpoint mapper
     */

    printf("registering bindings with endpoint mapper\n");

    rpc_ep_register(echo_v1_0_s_ifspec,
                    server_binding,
                    NULL,
                    (unsigned char *)"QDA application server",
                    &status);
    if (status)
    {
        if (protocol[0] && endpoint)
        {
            printf("rpc_ep_register: soft failure %d.\n", status);
        }
        else
        {
#ifndef _WIN32
        chk_dce_err(status, "rpc_ep_register()", "", 1);
#endif
            printf("rpc_ep_register: failed!\n");
            return 1;
        }
    }
    else
    {
        printf("registered.\n");
    }

    /*
     * Print out the servers endpoints (TCP and UDP port numbers)
     */

    printf ("Server's communications endpoints are:\n");

    for (i=0; i<RPC_FIELD_COUNT(server_binding); i++)
    {
        rpc_binding_to_string_binding(RPC_FIELD_BINDING_H(server_binding)[i],
                                      &string_binding,
                                      &status);
        if (string_binding)
        {
            printf("\t%s\n", (char *) string_binding);
            rpc_string_free(&string_binding, &status);
        }
    }

#ifndef _WIN32
    /*
     * Start the signal waiting thread in background. This thread will
     * Catch SIGINT and gracefully shutdown the server.
     */

    sigpipe_handler();
    wait_for_signals();
#endif

    /*
     * Begin listening for calls
     */

    printf ("listening for calls....\n");

    DCETHREAD_TRY
    {
        rpc_server_listen(rpc_c_listen_max_calls_default, &status);
    }
    DCETHREAD_CATCH_ALL(THIS_CATCH)
    {
        printf ("Server stoppped listening\n");
    }
    DCETHREAD_ENDTRY;

    /*
     * If we reached this point, then the server was stopped, most likely
     * by the signal handler thread called rpc_mgmt_stop_server().
     * gracefully cleanup and unregister the bindings from the
     * endpoint mapper.
     */

    printf ("Unregistering server from the endpoint mapper....\n");
    rpc_ep_unregister(echo_v1_0_s_ifspec,
                      server_binding,
                      NULL,
                      &status);
    chk_dce_err(status, "rpc_ep_unregister()", "", 0);

    rpc_binding_vector_free(&server_binding, &status);

    /*
     * retire the binding information
     */

    printf("Cleaning up communications endpoints...\n");
    rpc_server_unregister_if(echo_v1_0_s_ifspec,
                             NULL,
                             &status);
    chk_dce_err(status, "rpc_server_unregister_if()", "", 0);
    return(0);
}

#if 0
#ifndef _WIN32
static void
display_access_token(
    PACCESS_TOKEN pAccessToken)
{
    union
    {
        SID_AND_ATTRIBUTES user;
        BYTE buffer[sizeof(SID_AND_ATTRIBUTES) + SID_MAX_SIZE];
    } u;
    NTSTATUS status = 0;
    ULONG len = 0;
    PSTR sidstr = NULL;
    DWORD dwError = 0;
    DWORD dwSize = 0;
    PTOKEN_GROUPS pTokenInfo = NULL;
    DWORD iGroup = 0;
    PSTR *ppszSIDs = NULL;

    status = RtlQueryAccessTokenInformation(
        pAccessToken,
        TokenUser,
        &u.user,
        sizeof(u),
        &len);
    dwError = LwNtStatusToWin32Error(status);
    if (dwError)
    {
        printf("RtlQueryAccessTokenInformation() failed: %d, %d\n", status, dwError);
        goto error;
    }

    status = RtlAllocateCStringFromSid(
        &sidstr,
        u.user.Sid);
    dwError = LwNtStatusToWin32Error(status);
    if (dwError)
    {
        printf("RtlAllocateCStringFromSid() failed: %d, %d\n", status, dwError);
        goto error;
    }

    printf("client: %s\n", sidstr);

    status = RtlQueryAccessTokenInformation(
                     pAccessToken,
                     TokenGroups,
                     pTokenInfo,
                     dwSize,
                     &dwSize);
    dwError = LwNtStatusToWin32Error(status);
    if (dwError == ERROR_INSUFFICIENT_BUFFER)
    {
        dwError = 0;
    }
    if (dwError)
    {
        goto error;
    }

    pTokenInfo = calloc(dwSize, 1);
    if (!pTokenInfo)
    {
        dwError = ERROR_OUTOFMEMORY;
        goto error;
    }

    status = RtlQueryAccessTokenInformation(
                     pAccessToken,
                     TokenGroups,
                     pTokenInfo,
                     dwSize,
                     &dwSize);
    dwError = LwNtStatusToWin32Error(status);
    if (dwError)
    {
        printf("RtlQueryAccessTokenInformation() failed: %d, %d\n", status, dwError);
        goto error;
    }

    printf("Number of groups: %d\n", pTokenInfo->GroupCount);

    if (pTokenInfo->GroupCount)
    {
        ppszSIDs = calloc(sizeof(PSTR *) *pTokenInfo->GroupCount, 1);
        if (!ppszSIDs)
        {
            dwError = ERROR_OUTOFMEMORY;
            goto error;
        }

        for (iGroup = 0; iGroup < pTokenInfo->GroupCount; iGroup++)
        {
            status = RtlAllocateCStringFromSid(
                          &ppszSIDs[iGroup],
                          pTokenInfo->Groups[iGroup].Sid);
            dwError = LwNtStatusToWin32Error(status);
            if (dwError)
            {
                printf("RtlAllocateCStringFromSid() failed: %d, %d\n", status, dwError);
                goto error;
            }

            printf("%s\n", ppszSIDs[iGroup]);
        }

    }

cleanup:
    if (pTokenInfo)
    {
        free(pTokenInfo);
    }
    if (ppszSIDs)
    {
        free(ppszSIDs);
    }
    RTL_FREE(&sidstr);
    return;

error:
    goto cleanup;
}
#endif
#endif

/*=========================================================================
 *
 * Server implementation of ReverseIt()
 *
 *=========================================================================*/

idl_long_int SendBuffer(
    rpc_binding_handle_t h,
    unsigned char *buf,
    idl_long_int len)
{
    return len;
}

FILE *g_fp;


void SendFile_InPipe(
    /* [in] */ handle_t h,
    /* [in] */ BYTE_PIPE pipe_data)
{
    unsigned char pull_buf[2048];
    unsigned int transport_size = TRANSPORT_SIZE;

    if (!g_fp)
    {
        g_fp = fopen("/tmp/echo_server_pipe.dat", "w");
    }
    while(transport_size > 0)
    {
        pipe_data.pull(pipe_data.state,
                        pull_buf,
                        TRANSPORT_SIZE,
                        &transport_size);
        if (g_fp)
        {
            if (transport_size > 0)
            {
                fwrite(pull_buf, 1, transport_size, g_fp);
            }
            else
            {
                fclose(g_fp);
                g_fp = NULL;
            }
        }
    }
}

void SendFile_OutPipe(
    /* [in] */ handle_t h,
    /* [out] */ BYTE_PIPE *pipe_data)
{
    return; /* never called */
}


idl_boolean
ReverseIt(
    rpc_binding_handle_t h,
    args * in_text,
    args ** out_text,
    error_status_t * status
    )
{

    unsigned char * binding_info;
    error_status_t e;
    unsigned result_size;
    args * result;
    unsigned32 i,j,l;
    rpc_authz_cred_handle_t hPriv = { 0 };
    unsigned32 dwProtectLevel = 0;
    unsigned32 rpc_status = rpc_s_ok;
    unsigned char *authPrinc = NULL;
#if 0
    PACCESS_TOKEN token = NULL;
    rpc_transport_info_handle_t transport_info = NULL;
    unsigned32 rpcstatus = 0;
    unsigned char* sesskey = NULL;
    unsigned32 sesskey_len = 0;
#endif

    /*
     * Get some info about the client binding
     */
    rpc_binding_to_string_binding(h, &binding_info, &e);
    if (e == rpc_s_ok)
    {
        printf ("ReverseIt() called by client: %s\n", (char *) binding_info);
        rpc_string_free(&binding_info, status);
        binding_info = NULL;
    }

    rpc_binding_inq_auth_caller(
        h,
        &hPriv,
        &authPrinc,
        &dwProtectLevel,
        NULL, /* unsigned32 *authn_svc, */
        NULL, /* unsigned32 *authz_svc, */
        &rpc_status);
    if (rpc_status == rpc_s_ok)
    {
        printf("rpc_binding_inq_auth_caller: sts=%d authPrinc=%s prot=%d\n",
               rpc_status, (char*) authPrinc, dwProtectLevel);
        rpc_string_free(&authPrinc, status);
    }

#if 0
    rpc_binding_inq_access_token_caller(
            h,
            &token,
            &rpc_status);
    if (rpc_status == rpc_s_ok)
    {
        display_access_token(token);
    }
#endif

#if 0
    rpc_binding_inq_transport_info(h, &transport_info, &rpcstatus);

    if (transport_info)
    {
        rpc_smb_transport_info_inq_session_key(transport_info, &sesskey, &sesskey_len);

        printf ("Session key: ");

        for (i = 0; i < sesskey_len; i++)
        {
            printf("%X", sesskey[i]);
        }

        printf ("\n");
    }
#endif

    if (in_text == NULL) return 0;

    /*
     *  Print the in_text
     */

    printf("\n\nFunction ReverseIt() -- input argments\n");

    for (i=0; i<in_text->argc; i++)
        printf("\t[arg %d]: %s\n", i, in_text->argv[i]);

    printf ("\n=========================================\n");

    /*
     * Allocate the output args as dynamic storage bound
     * to this RPC. The output args are the same size as the
     * input args since we are simply reversing strings.
     */

    result_size = sizeof(args) + in_text->argc * sizeof(string_t *);
    result = (args * )rpc_ss_allocate(result_size);
    result->argc = in_text->argc;

    for (i=0; i < in_text->argc; i++)
    {
        result->argv[i] =
            (string_t)rpc_ss_allocate((idl_size_t) strlen((const char *) in_text->argv[i]) + 1);
    }

    /*
     * do the string reversal
     */

    for (i=0; i < in_text->argc; i++)
    {
        l = (unsigned32) strlen((const char *) in_text->argv[i]);
        for (j=0; j<l; j++)
        {
            result->argv[i][j] = in_text->argv[i][l-j-1];
        }
        result->argv[i][l]=0;           /* make sure its null terminated! */
    }

    *out_text = result;
    *status = error_status_ok;

    return 1;
}


#ifndef _WIN32
/*=========================================================================
 *
 * wait_for_signals()
 *
 *
 * Set up the process environment to properly deal with signals.
 * By default, we isolate all threads from receiving asynchronous
 * signals. We create a thread that handles all async signals.
 * The signal handling actions are handled in the handler thread.
 *
 * For AIX, we cant use a thread that sigwaits() on a specific signal,
 * we use a plain old, lame old Unix signal handler.
 *
 *=========================================================================*/
void *pfn_sigpipe(void *ctx)
{
    sigset_t sm;
    int sig = 0;

    sigemptyset(&sm);
    sigaddset(&sm, SIGPIPE);

    for (;;)
    {
        fprintf(stderr, "Waiting for SIGPIPE...\n");
        sigwait(&sm, &sig);
        fprintf(stderr, "SIGPIPE caught!\n");
    }
}

void sigpipe_handler(void)
{
    sigset_t sm;
    pthread_t th;

    sigemptyset(&sm);
    sigaddset(&sm, SIGPIPE);
    pthread_sigmask(SIG_BLOCK,  &sm, NULL);
    pthread_create(&th, NULL, pfn_sigpipe, NULL);
}


void
wait_for_signals()
{
    sigset_t signals;

    sigemptyset(&signals);
    sigaddset(&signals, SIGINT);

    dcethread_signal_to_interrupt(&signals, dcethread_self());
}

#endif
