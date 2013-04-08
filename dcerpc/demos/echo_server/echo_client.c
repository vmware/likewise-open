/* ex: set shiftwidth=4 softtabstop=4 expandtab: */
/*
 * echo_client  : demo DCE RPC application
 *
 * Jim Doyle, jrd@bu.edu, 09-05-1998
 *
 *
 */
#if HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#ifndef _WIN32
#include <unistd.h>
#endif
#include "echo.h"
#include <misc.h>

#include <getopt.h>

#define MAX_USER_INPUT 128
#define MAX_LINE 100 * 1024

#ifdef _WIN32
#define strdup _strdup
#define EOF_STRING "^Z"
#else
#define EOF_STRING "^D"
#endif

static char *argv0;
/*
 * Forward declarations
 */

static int
get_client_rpc_binding(
    rpc_binding_handle_t * binding_handle,
    rpc_if_handle_t interface_spec,
    char * hostname,
    char * protocol,
    char * endpoint
    );

/*
 * usage()
 */

static void usage(char *argv0, const char *msg)
{
    if (msg)
    {
        printf("%s\n", msg);
    }

    printf("usage: %s [-h hostname] [-a name [-p level]] [-e endpoint] [-n] [-u] [-t]\n", argv0);
    printf("         -h:  specify host of RPC server (default is localhost)\n");
    printf("         -a:  specify authentication identity\n");
    printf("         -p:  specify protection level\n");
    printf("         -e:  specify endpoint for protocol\n");
    printf("         -n:  use named pipe protocol\n");
    printf("         -u:  use UDP protocol\n");
    printf("         -t:  use TCP protocol (default)\n");
    printf("         -g:  instead of prompting, generate a data string of the specified length\n");
    printf("         -d:  turn on debugging\n");
    printf("\n");
    exit(1);
}


typedef struct _PROG_ARGS
{
    char *rpc_host;
    char *spn;
    unsigned32 protect_level;
    char *endpoint;
    char *protocol;
    int generate_length;
} PROG_ARGS;


void
parseArgs(
    int argc,
    char *argv[],
    PROG_ARGS *args,
    int *params)
{
    int i;

    i = 1;
    args->protocol = PROTOCOL_TCP;
    while (i<argc && argv[i][0] == '-')
    {
        if (strcmp("-h", argv[i]) == 0)
        {
            i++;
            if (i >= argc)
            {
                usage(argv0, "-h hostname/IP address missing");
            }
            args->rpc_host = strdup(argv[i]);
            i++;
        }
        else if (strcmp("-a", argv[i]) == 0)
        {
            i++;
            if (i >= argc)
            {
                usage(argv0, "-a Service Principal Name missing");
            }
            args->spn = strdup(argv[i]);
            
            i++;
        }
        else if (strcmp("-p", argv[i]) == 0)
        {
            i++;
            if (i >= argc)
            {
                usage(argv0, "-p Protection level  missing");
            }
            args->protect_level = atoi(argv[i]);
            i++;
        }
        else if (strcmp("-e", argv[i]) == 0)
        {
            i++;
            if (i >= argc)
            {
                usage(argv0, "-e Endpoint missing");
            }
            args->endpoint = strdup(argv[i]);
            i++;
        }
        else if (strcmp("-n", argv[i]) == 0)
        {
            args->protocol = PROTOCOL_NP;
            i++;
        }
        else if (strcmp("-u", argv[i]) == 0)
        {
            args->protocol = PROTOCOL_UDP;
            i++;
        }
        else if (strcmp("-t", argv[i]) == 0)
        {
            args->protocol = PROTOCOL_TCP;
            i++;
        }
        else if (strcmp("-g", argv[i]) == 0)
        {
            i++;
            if (i >= argc)
            {
                usage(argv0, "-g Generate length missing");
            }
            args->generate_length = atoi(argv[i]);
            i++;
        }
        else
        {
            usage(argv0, "Unknown option");
        }
    }

    if (i<argc)
    {
        *params = i;
    }
}



int
main(
    int argc,
    char *argv[]
    )
{

    /*
     * command line processing and options stuff
     */

    PROG_ARGS progArgs = {0};

    char buf[MAX_LINE+1];

    /*
     * stuff needed to make RPC calls
     */

    unsigned32 status;
    rpc_binding_handle_t echo_server;
    args * inargs;
    args * outargs;
    int ok;
    unsigned32 i;
    int params;

    char * nl;

    argv0 = argv[0];
    progArgs.generate_length = -1;
    /*
     * Process the cmd line args
     */
    parseArgs(argc, argv, &progArgs, &params);

    /*
     * Get a binding handle to the server using the following params:
     *
     *  1. the hostname where the server lives
     *  2. the interface description structure of the IDL interface
     *  3. the desired transport protocol (UDP or TCP)
     */

    if (get_client_rpc_binding(&echo_server,
                               echo_v1_0_c_ifspec,
                               progArgs.rpc_host,
                               progArgs.protocol,
                               progArgs.endpoint) == 0)
    {
        printf ("Couldnt obtain RPC server binding. exiting.\n");
        return(1);
    }

    if (progArgs.spn)
    {
        rpc_binding_set_auth_info(echo_server,
            (unsigned char *) progArgs.spn,
            progArgs.protect_level,
            rpc_c_authn_gss_negotiate,
            NULL,
            rpc_c_authz_name, &status);
        if (status)
        {
            printf ("Couldn't set auth info %u. exiting.\n", status);
            return(1);
        }
    }

    /*
     * Allocate an "args" struct with enough room to accomodate
     * the max number of lines of text we can can from stdin.
     */

    inargs = (args *)malloc(sizeof(args) + MAX_USER_INPUT * sizeof(string_t));
    if (inargs == NULL) printf("FAULT. Didnt allocate inargs.\n");

    if (progArgs.generate_length < 0)
    {
        /*
         * Get text from the user and pack into args.
         */

        printf ("enter stuff (%s on an empty line when done):\n\n\n", EOF_STRING);
        i = 0;
        while (!feof(stdin) && i < MAX_USER_INPUT )
        {
            if (NULL==fgets(buf, MAX_LINE, stdin))
                break;
            if ((nl=strchr(buf, '\n')))                   /* strip the newline */
                *nl=0;
            inargs->argv[i] = (string_t)strdup(buf);      /* copy from buf */
            i++;
        }
        inargs->argc = i;
    }
    else
    {
        inargs->argv[0] = malloc(progArgs.generate_length + 1);
        inargs->argv[0][0] = 's';

        for(i = 1; i < (unsigned long)progArgs.generate_length; i++)
        {
            inargs->argv[0][i] = i%10 + '0';
        }

        if(progArgs.generate_length > 0)
            inargs->argv[0][progArgs.generate_length - 1] = 'e';
        inargs->argv[0][progArgs.generate_length] = '\0';
        inargs->argc = 1;
    }

    /*
     * Do the RPC call
     */

    printf ("calling server\n");
    ok = ReverseIt(echo_server, inargs, &outargs, &status);

    /*
     * Print the results
     */

    if (ok && status == error_status_ok)
    {
        printf ("got response from server. results: \n");
        for (i=0; i<outargs->argc; i++)
            printf("\t[%u]: %s\n", i, outargs->argv[i]);
        printf("\n===================================\n");

    }

    if (status != error_status_ok)
        chk_dce_err(status, "ReverseIt()", "main()", 1);

    /*
     * Done. Now gracefully teardown the RPC binding to the server
     */

    rpc_binding_free(&echo_server, &status);
    return(0);

}

/*==========================================================================
 *
 * get_client_rpc_binding()
 *
 *==========================================================================
 *
 * Gets a binding handle to an RPC interface.
 *
 * parameters:
 *
 *    [out]     binding_handle
 *    [in]      interface_spec <- DCE Interface handle for service
 *    [in]      hostname       <- Internet hostname where server lives
 *    [in]      protocol       <- "ncacn_ip_tcp", etc.
 *    [in]      endpoint       <- optional
 *
 *==========================================================================*/

static int
get_client_rpc_binding(
    rpc_binding_handle_t * binding_handle,
    rpc_if_handle_t interface_spec,
    char * hostname,
    char * protocol,
    char * endpoint
    )
{
    char * string_binding = NULL;
    error_status_t status;

    /*
     * create a string binding given the command line parameters and
     * resolve it into a full binding handle using the endpoint mapper.
     *  The binding handle resolution is handled by the runtime library
     */

    rpc_string_binding_compose(NULL,
			       (unsigned char *) protocol,
			       (unsigned char *) hostname,
			       (unsigned char *) endpoint,
			       NULL,
			       (unsigned char **) &string_binding,
			       &status);
    chk_dce_err(status, "rpc_string_binding_compose()", "get_client_rpc_binding", 1);


    rpc_binding_from_string_binding((unsigned char *)string_binding,
                                    binding_handle,
                                    &status);
    chk_dce_err(status, "rpc_binding_from_string_binding()", "get_client_rpc_binding", 1);

#if !defined(_WIN32)
/* Don't use endpoint mapper for now, must supply endpoint on command line */
    if (!endpoint)
    {
        /*
         * Resolve the partial binding handle using the endpoint mapper
         */

        rpc_ep_resolve_binding(*binding_handle,
                               interface_spec,
                               &status);
        chk_dce_err(status, "rpc_ep_resolve_binding()", "get_client_rpc_binding", 1);
    }
#endif

    rpc_string_free((unsigned char **) &string_binding, &status);
    chk_dce_err(status, "rpc_string_free()", "get_client_rpc_binding", 1);

    /*
     * Get a printable rendition of the binding handle and echo to
     * the user.
     */

    rpc_binding_to_string_binding(*binding_handle,
                                  (unsigned char **)&string_binding,
                                  &status);
    chk_dce_err(status, "rpc_binding_to_string_binding()", "get_client_rpc_binding", 1);

    printf("fully resolving binding for server is: %s\n", string_binding);

    rpc_string_free((unsigned char **) &string_binding, &status);
    chk_dce_err(status, "rpc_string_free()", "get_client_rpc_binding", 1);

    return 1;
}
