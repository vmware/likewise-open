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

#include  <dce/dcethread.h>

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <errno.h>
#ifndef _WIN32
#include <unistd.h>
#endif
#include "echo.h"
#include <misc.h>

#include <gssapi/gssapi.h>
#include <gssapi/gssapi_ext.h>

extern int FIPS_mode_set(int);


/* Defines related to GSS authentication */

#ifndef GSSAPI_MECH_SPNEGO
/*
 * SPNEGO MECH OID: 1.3.6.1.5.5.2
 * http://www.oid-info.com/get/1.3.6.1.5.5.2
 */
#define GSSAPI_MECH_SPNEGO "\x2b\x06\x01\x05\x05\x02"
#define GSSAPI_MECH_SPNEGO_LEN 6
#endif

#ifndef GSSAPI_SRP_CRED_OPT_PW
/*
 * vmwSrpCredOptPwd: 1.3.6.1.4.6876.11711.2.1.1.1
 * http://www.oid-info.com/get/1.3.6.1.4.6876.11711.2.1.1.1
 */
#define GSSAPI_SRP_CRED_OPT_PW  \
    "\x2b\x06\x01\x04\x01\xb5\x5c\xdb\x3f\x02\x01\x01\x01"
#define GSSAPI_SRP_CRED_OPT_PW_LEN 13
#endif

#ifndef GSSAPI_NTLM_CRED_OPT_PW
/*
 * 1.3.6.1.4.1.27433.3.1
 * http://www.oid-info.com/get/1.3.6.1.4.1.27433
 */
#define GSSAPI_NTLM_CRED_OPT_PW "\x2b\x06\x01\x04\x01\x81\xd6\x29\x03\x01"
#define GSSAPI_NTLM_CRED_OPT_PW_LEN 10
#endif

/*
 * 1.3.6.1.4.1.6876.11711.2.1.2.1
 *
 * {iso(1) identified-organization(3) dod(6) internet(1) private(4)
 *   enterprise(1) 6876 vmwSecurity(11711) vmwAuthentication(2) vmwGSSAPI(1)
 *   vmwUNIX(2) vmwSrpCredOptPwd(1)}
 * Official registered GSSAPI_UNIX password cred option OID
 */
#ifndef GSSAPI_UNIX_CRED_OPT_PW
#define GSSAPI_UNIX_CRED_OPT_PW  \
    "\x2b\x06\x01\x04\x01\xb5\x5c\xdb\x3f\x02\x01\x02\x01"
#define GSSAPI_UNIX_CRED_OPT_PW_LEN  13
#endif

typedef struct _ntlm_auth_identity
{
    char          *User;
    unsigned int   UserLength;
    char          *Domain;
    unsigned int   DomainLength;
    char          *Password;
    unsigned int   PasswordLength;
    unsigned int   Flags;
} ntlm_auth_identity;

#define GSSAPI_NTLM_AUTH_IDENTITY_UNICODE  0
#define GSSAPI_NTLM_AUTH_IDENTITY_ANSI     1

#define MAX_USER_INPUT 128
#define MAX_LINE 100 * 1024

#ifdef _WIN32
#define strdup _strdup
#define snprintf _snprintf
#define sleep(s) Sleep((s) * 1000)
#define EOF_STRING "^Z"
#else
#define EOF_STRING "^D"
#endif

#define DO_RPC(rpc_pfn, sts) \
  do {                       \
    dcethread_exc *exc;      \
    DCETHREAD_TRY            \
    {                        \
      exc = NULL;            \
      (sts) = rpc_pfn;       \
    }                        \
    DCETHREAD_CATCH_ALL(exc) \
    {                        \
      (sts) = dcethread_exc_getstatus(exc); \
    }                        \
    DCETHREAD_ENDTRY         \
  } while (0)

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
    char * endpoint,
    int silent
    );

/*
 * usage()
 */

static void usage(char *argv0, const char *msg)
{
    if (msg)
    {
        printf("ERROR: %s\n\n", msg);
    }

#ifndef _WIN32
    printf("usage: %s [-h hostname] [-a name [-p level]] [-e endpoint] [-l] [-n] [-u] [-t][--loop N][-g N][-d][--pwd pwd][--threads num][--srp|--ntlm]\n", argv0);
#else
    printf("usage: %s [-h hostname] [-a name [-p level]] [-e endpoint] [-l] [-n] [-u] [-t][--loop N][-g N][-d][--pwd pwd][--threads num][--srp]\n", argv0);
#endif
    printf("            -h: specify host of RPC server; option stacks (default is localhost)\n");
    printf("            -a: specify authentication identity\n");
    printf("            -p: specify protection level\n");
    printf("            -e: specify endpoint for protocol\n");
    printf("            -l: use ncalpc protocol\n");
    printf("            -n: use named pipe protocol\n");
    printf("            -u: use UDP protocol\n");
    printf("            -t: use TCP protocol (default)\n");
    printf("            -g: instead of prompting, generate a data string of the specified length\n");
    printf("            -d: turn on debugging\n");
    printf("        --loop: loop Reverseit N times\n");
    printf("  --loop-sleep: Delay N Seconds before repeating RPC call (--loop > 1)\n");
    printf("      --rebind: Create new RPC binding handle N times\n");
    printf("--rebind-sleep: Delay N Seconds before acting on --rebind option\n");
    printf("         --srp: authenticate using SRP mechanism\n");
#ifndef _WIN32
    printf("        --ntlm: authenticate using NTLM mechanism\n");
#endif
    printf("         --pwd: password for authentication identity\n");
    printf("     --pwd-bad: bad password to test bad/good authentication\n");
    printf("     --threads: Call ReverseIt in 'num'; default --threads 10, --loop 1000\n");
    printf("     --silent:  Do not print server responses\n");
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
    int do_srp;
    int do_unix;
    int do_ntlm;
    int loop;
    int rebind_count;
    int rebind_sleep;
    int loop_sleep;
    char *passwd;
    char *passwd_bad;
    int do_threads;
    int num_threads;
    int silent;
    args *inargs;
} PROG_ARGS;

typedef struct _ASSOC_ARGS
{
    int cnt;
    PROG_ARGS *progArgs;
} ASSOC_ARGS;

typedef struct _threadarg
{
    rpc_binding_handle_t echo_server;
    int num_threads;
    args *inargs;
    PROG_ARGS *progArgs;
    unsigned32 status;
} threadarg;

static unsigned32
create_binding_handle(
    rpc_binding_handle_t * ret_binding_handle,
    rpc_if_handle_t interface_spec,
    PROG_ARGS *progArgs,
    unsigned32 rebind);

int
parseArgs(
    int argc,
    char *argv[],
    ASSOC_ARGS *assocs,
    int *params)
{
    int i = 0;
    int assoc_cnt = 1; /* Assume possibility no -h is present */
    PROG_ARGS args_buf = {0};
    PROG_ARGS *args = &args_buf;
    PROG_ARGS *ret_args = NULL;
    int iassoc = 0;

    /* Pre-scan arguments for all -h options */
    for (i = 0; i < argc; i++)
    {
        if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "-l") == 0)
        {
            assoc_cnt++;
        }
    }

    ret_args = (PROG_ARGS *) calloc(assoc_cnt, sizeof(PROG_ARGS));
    if (!args)
    {
        return errno;
    }

    i = 1;
    args->protocol = PROTOCOL_TCP;
    args->generate_length = -1;
    while (i<argc && argv[i][0] == '-')
    {
        if (strcmp("-h", argv[i]) == 0)
        {
            if (i > 1)
            {
                /*
                 * Switch to the next association when -h is found.
                 * Assumption here is -h is always the first argument.
                 */
                ret_args[iassoc++] = *args;

                /* Reset previously parsed arguments since last -h option */
                memset(args, 0, sizeof(*args));
                args->protocol = PROTOCOL_TCP;
                args->generate_length = -1;
            }
            i++;
            if (i >= argc)
            {
                usage(argv0, "-h hostname/IP address missing");
            }
            if (args->rpc_host)
            {
                usage(argv0, "-h option previously specified");
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
            if (args->spn)
            {
                usage(argv0, "-a option previously specified");
            }
            args->spn = strdup(argv[i]);

            i++;
        }
        else if (strcmp("-p", argv[i]) == 0)
        {
            /*
             * DCE/RPC protection levels (from dcerpc/include/dce/rpcbase.idl):
             * rpc_c_protect_level_default         = 0; * default for auth svc        *
             * rpc_c_protect_level_none            = 1; * no authentication performed *
             * rpc_c_protect_level_connect         = 2; * only on "connect"           *
             * rpc_c_protect_level_call            = 3; * on first pkt of each call   *
             * rpc_c_protect_level_pkt             = 4; * on each packet              *
             * rpc_c_protect_level_pkt_integ       = 5; * strong integrity check      *
             * rpc_c_protect_level_pkt_privacy     = 6; * encrypt arguments           *
             */

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
            if (args->endpoint)
            {
                usage(argv0, "-e option previously specified");
            }
            args->endpoint = strdup(argv[i]);
            i++;
        }
        else if (strcmp("-l", argv[i]) == 0)
        {
            args->protocol = PROTOCOL_NCALRPC;
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
        else if (strcmp("--loop", argv[i]) == 0)
        {
            i++;
            if (i >= argc)
            {
                usage(argv0, "--loop count missing");
            }
            args->loop = atoi(argv[i]);
            i++;
	}
        else if (strcmp("--rebind", argv[i]) == 0)
        {
            i++;
            if (i >= argc)
            {
                usage(argv0, "--rebind count missing");
            }
            args->rebind_count = atoi(argv[i]);
            i++;
	}
        else if (strcmp("--rebind-sleep", argv[i]) == 0)
        {
            i++;
            if (i >= argc)
            {
                usage(argv0, "--rebind-sleep delay (in sec) missing");
            }
            args->rebind_sleep = atoi(argv[i]);
            i++;
        }
        else if (strcmp("--loop-sleep", argv[i]) == 0)
        {
            i++;
            if (i >= argc)
            {
                usage(argv0, "--loop-sleep count missing");
            }
            args->loop_sleep = atoi(argv[i]);
            i++;
        }
        else if (strcmp("--srp", argv[i]) == 0)
        {
            if (args->do_srp)
            {
                usage(argv0, "--srp option previously specified");
            }
            args->do_srp = 1;
            i++;
        }
        else if (strcmp("--unix", argv[i]) == 0)
        {
            if (args->do_srp)
            {
                usage(argv0, "--unix option previously specified");
            }
            args->do_unix = 1;
            i++;
        }
        else if (strcmp("--ntlm", argv[i]) == 0)
        {
            if (args->do_ntlm)
            {
                usage(argv0, "--ntlm option previously specified");
            }
            args->do_ntlm = 1;
            i++;
        }
        else if (strcmp("--pwd", argv[i]) == 0)
        {
            i++;
            if (i >= argc)
            {
                usage(argv0, "--pwd value missing");
            }
            if (args->passwd)
            {
                usage(argv0, "--pwd option previously specified");
            }
            else if (strlen(argv[i]) == 1 && argv[i][0] == '-')
            {
                char pwdbuf[256];
                printf("password: ");
                fflush(stdout);
                fgets(pwdbuf, sizeof(pwdbuf), stdin);
                pwdbuf[strlen(pwdbuf) - 1] = '\0';
                printf("password is <%s>\n", pwdbuf);
                args->passwd = strdup(pwdbuf);
            }
            else
            {
                args->passwd = strdup(argv[i]);
            }
            i++;
        }
        else if (strcmp("--pwd-bad", argv[i]) == 0)
        {
            i++;
            if (i >= argc)
            {
                usage(argv0, "--pwd-bad value missing");
            }
            if (args->passwd_bad)
            {
                usage(argv0, "--pwd-bad option previously specified");
            }
            args->passwd_bad = strdup(argv[i]);
            i++;
        }
        else if (strcmp("--threads", argv[i]) == 0)
        {
            args->do_threads = 1;
            args->num_threads = 10;
            i++;

            if (i < argc && isdigit((int) argv[i][0]))
            {
                args->num_threads = atoi(argv[i]);
                i++;
            }
        }
        else if (strcmp("--silent", argv[i]) == 0)
        {
            args->silent++;
            i++;
        }
        else
        {
            usage(argv0, "Unknown option");
        }
    }
    ret_args[iassoc++] = *args;

    for (i=0; i<iassoc; i++)
    {
        args = &ret_args[i];
        if (args->rebind_count <= 0)
        {
            args->rebind_count = 1;
        }
        if (args->rebind_count == 1 && args->passwd_bad)
        {
            usage(argv0, "--pwd-bad has no effect without specifying --rebind > 1");
        }
        if (!args->endpoint)
        {
            args->endpoint = strdup("31415");
        }
        if (args->loop == 0)
        {
            args->loop = 1;
        }
        if (i<argc)
        {
            *params = i;
        }
    }
    assocs->cnt = iassoc;
    assocs->progArgs = ret_args;
    return 0;
}

void freeArgs(
    PROG_ARGS *args)
{
    if (args->rpc_host)
    {
        free(args->rpc_host);
    }
    if (args->spn)
    {
        free(args->spn);
    }
    if (args->endpoint)
    {
        free(args->endpoint);
    }
    if (args->passwd)
    {
        free(args->passwd);
    }
    if (args->passwd_bad)
    {
        free(args->passwd_bad);
    }
}


void freeInargs(
    args *args)
{
    size_t i = 0;

    if (!args)
    {
        return;
    }
    for (i=0; i < args->argc; i++)
    {
        free(args->argv[i]);
    }
    free(args);
}

void freeOutargs(
    args **pargs)
{
    size_t i = 0;
    unsigned32 sts = 0;
    args *args = NULL;

    if (!pargs || !*pargs)
    {
        return;
    }
    args = *pargs;
    for (i=0; i < args->argc; i++)
    {
        rpc_sm_client_free(args->argv[i], &sts);
    }
    rpc_sm_client_free(args, &sts);
    *pargs = NULL;
}

unsigned32
rpc_create_srp_auth_identity(
    const char *user,
    const char *domain,
    const char *password,
    rpc_auth_identity_handle_t *rpc_identity_h)
{
    OM_uint32 min = 0;
    OM_uint32 maj = 0;
    const gss_OID_desc gss_srp_password_oid =
        {GSSAPI_SRP_CRED_OPT_PW_LEN, (void *) GSSAPI_SRP_CRED_OPT_PW};
    gss_OID_desc spnego_mech_oid =
        {GSSAPI_MECH_SPNEGO_LEN, (void *) GSSAPI_MECH_SPNEGO};
    gss_buffer_desc name_buf = {0};
    gss_name_t gss_name_buf = NULL;
    gss_buffer_desc gss_pwd = {0};
    size_t upn_len = 0;
    char *upn = NULL;
    gss_cred_id_t cred_handle = NULL;
    gss_OID_desc mech_oid_array[1];
    gss_OID_set_desc desired_mechs = {0};

    if (domain)
    {
        /* user@DOMAIN\0 */
        upn_len = strlen(user) + 1 + strlen(domain) + 1;
        upn = calloc(upn_len, sizeof(char));
        if (!upn)
        {
            maj = GSS_S_FAILURE;
            min = rpc_s_no_memory;
        }
        snprintf(upn, upn_len, "%s@%s", user, domain);
    }
    else
    {
        /* Assume a UPN-like name form when no domain is provided */
        upn = (char *) user;
    }
    name_buf.value = upn;
    name_buf.length = strlen(name_buf.value);
    maj = gss_import_name(
              &min,
              &name_buf,
              GSS_C_NT_USER_NAME,
              &gss_name_buf);
    if (maj)
    {
        goto error;
    }

    /*
     * Use SPNEGO mech OID to acquire cred
     */
    desired_mechs.count = 1;
    desired_mechs.elements = mech_oid_array;
    desired_mechs.elements[0] = spnego_mech_oid;
    maj = gss_acquire_cred(
              &min,
              gss_name_buf,
              0,
              &desired_mechs,
              GSS_C_INITIATE,
              &cred_handle,
              NULL,
              NULL);
    if (maj)
    {
        goto error;
    }

    gss_pwd.value = (char *) password;
    gss_pwd.length = strlen(gss_pwd.value);

    maj = gss_set_cred_option(
              &min,
              &cred_handle,
              (gss_OID) &gss_srp_password_oid,
              &gss_pwd);
    if (maj)
    {
        goto error;
    }

    *rpc_identity_h = (rpc_auth_identity_handle_t) cred_handle;

error:
    if (maj)
    {
        maj = min ? min : maj;
    }

    if (upn != user)
    {
        free(upn);
    }
    if (gss_name_buf)
    {
        gss_release_name(&min, &gss_name_buf);
    }

    return maj;
}

unsigned32
rpc_create_unix_auth_identity(
    const char *user,
    const char *domain,
    const char *password,
    rpc_auth_identity_handle_t *rpc_identity_h)
{
    OM_uint32 min = 0;
    OM_uint32 maj = 0;
    const gss_OID_desc gss_unix_password_oid =
        {GSSAPI_UNIX_CRED_OPT_PW_LEN, (void *) GSSAPI_UNIX_CRED_OPT_PW};
    gss_OID_desc spnego_mech_oid =
        {GSSAPI_MECH_SPNEGO_LEN, (void *) GSSAPI_MECH_SPNEGO};
    gss_buffer_desc name_buf = {0};
    gss_name_t gss_name_buf = NULL;
    gss_buffer_desc gss_pwd = {0};
    size_t upn_len = 0;
    char *upn = NULL;
    gss_cred_id_t cred_handle = NULL;
    gss_OID_desc mech_oid_array[1];
    gss_OID_set_desc desired_mechs = {0};

    if (domain)
    {
        /* user@DOMAIN\0 */
        upn_len = strlen(user) + 1 + strlen(domain) + 1;
        upn = calloc(upn_len, sizeof(char));
        if (!upn)
        {
            maj = GSS_S_FAILURE;
            min = rpc_s_no_memory;
        }
        snprintf(upn, upn_len, "%s@%s", user, domain);
    }
    else
    {
        /* Assume a UPN-like name form when no domain is provided */
        upn = (char *) user;
    }
    name_buf.value = upn;
    name_buf.length = strlen(name_buf.value);
    maj = gss_import_name(
              &min,
              &name_buf,
              GSS_C_NT_ANONYMOUS,
              &gss_name_buf);
    if (maj)
    {
        goto error;
    }

    /*
     * Use SPNEGO mech OID to acquire cred
     */
    desired_mechs.count = 1;
    desired_mechs.elements = mech_oid_array;
    desired_mechs.elements[0] = spnego_mech_oid;
    maj = gss_acquire_cred(
              &min,
              gss_name_buf,
              0,
              &desired_mechs,
              GSS_C_INITIATE,
              &cred_handle,
              NULL,
              NULL);
    if (maj)
    {
        goto error;
    }

    gss_pwd.value = (char *) password;
    gss_pwd.length = strlen(gss_pwd.value);

    maj = gss_set_cred_option(
              &min,
              &cred_handle,
              (gss_OID) &gss_unix_password_oid,
              &gss_pwd);
    if (maj)
    {
        goto error;
    }

    *rpc_identity_h = (rpc_auth_identity_handle_t) cred_handle;

error:
    if (maj)
    {
        maj = min ? min : maj;
    }

    if (upn != user)
    {
        free(upn);
    }
    if (gss_name_buf)
    {
        gss_release_name(&min, &gss_name_buf);
    }

    return maj;
}

unsigned32
rpc_create_ntlm_auth_identity(
    const char *user,
    const char *domain,
    const char *password,
    rpc_auth_identity_handle_t *rpc_identity_h)
{
    OM_uint32 min = 0;
    OM_uint32 maj = 0;
    const gss_OID_desc gss_ntlm_cred_opt_pw_oid =
        {GSSAPI_NTLM_CRED_OPT_PW_LEN, (void *) GSSAPI_NTLM_CRED_OPT_PW};
    gss_OID_desc spnego_mech_oid =
        {GSSAPI_MECH_SPNEGO_LEN, (void *) GSSAPI_MECH_SPNEGO};
    ntlm_auth_identity ntlm_identity = {0};
    gss_name_t gss_name_buf = NULL;
    gss_buffer_desc gss_pwd = {0};
    gss_buffer_desc name_buf = {0};
    char *upn = NULL;
    size_t upn_len = 0;
    gss_cred_id_t cred_handle = NULL;
    gss_OID_desc mech_oid_array[1];
    gss_OID_set_desc desired_mechs = {0};

    if (domain)
    {
        /* user@DOMAIN\0 */
        upn_len = strlen(user) + 1 + strlen(domain) + 1;
        upn = calloc(upn_len, sizeof(char));
        if (!upn)
        {
            maj = GSS_S_FAILURE;
            min = rpc_s_no_memory;
        }
        snprintf(upn, upn_len, "%s@%s", user, domain);
    }

    name_buf.value = upn;
    name_buf.length = strlen(name_buf.value);
    maj = gss_import_name(
              &min,
              &name_buf,
              GSS_C_NT_USER_NAME,
              &gss_name_buf);
    if (maj)
    {
        goto error;
    }

    /*
     * Use SPNEGO mech OID to acquire cred
     */
    desired_mechs.count = 1;
    desired_mechs.elements = mech_oid_array;
    desired_mechs.elements[0] = spnego_mech_oid;
    maj = gss_acquire_cred(
              &min,
              gss_name_buf,
              0,
              &desired_mechs,
              GSS_C_INITIATE,
              &cred_handle,
              NULL,
              NULL);
    if (maj)
    {
        goto error;
    }

    ntlm_identity.User = (char *)user;
    ntlm_identity.UserLength = (int) strlen(ntlm_identity.User);
    ntlm_identity.Domain = (char *)domain;
    ntlm_identity.DomainLength = (int) strlen(ntlm_identity.Domain);
    ntlm_identity.Password = (char *)password;
    ntlm_identity.PasswordLength = (int) strlen(ntlm_identity.Password);
    ntlm_identity.Flags = GSSAPI_NTLM_AUTH_IDENTITY_ANSI;

    gss_pwd.value = (char *) &ntlm_identity;
    gss_pwd.length = sizeof(ntlm_identity);

    maj = gss_set_cred_option(
              &min,
              &cred_handle,
              (gss_OID) &gss_ntlm_cred_opt_pw_oid,
              &gss_pwd);
    if (maj)
    {
        goto error;
    }

    *rpc_identity_h = (rpc_auth_identity_handle_t) cred_handle;

error:
    if (maj)
    {
        maj = min ? min : maj;
    }

    if (upn != user)
    {
        free(upn);
    }
    if (gss_name_buf)
    {
        gss_release_name(&min, &gss_name_buf);
    }

    return maj;
}


unsigned32
create_rpc_identity(
    PROG_ARGS *progArgs,
    char *passwd,
    rpc_binding_handle_t rpc_binding_h)
{
    rpc_auth_identity_handle_t rpc_identity = NULL;
    unsigned32 serr = 0;
    char *at = NULL;
    char *username = NULL;
    char *domain = NULL;

    if (progArgs->spn)
    {
        username = strdup(progArgs->spn);
        at = strchr(username, '@');
        if (at)
        {
            *at = '\0';
            domain = at + 1;
        }

        if (progArgs->do_srp)
        {
            serr = rpc_create_srp_auth_identity(
                      username,
                      domain,
                      passwd,
                      &rpc_identity);
            if (serr)
            {
                goto error;
            }
        }
        else if (progArgs->do_unix)
        {
            serr = rpc_create_unix_auth_identity(
                      username,
                      domain,
                      passwd,
                      &rpc_identity);
            if (serr)
            {
                goto error;
            }
        }
        else if (progArgs->do_ntlm)
        {
            serr = rpc_create_ntlm_auth_identity(
                      username,
                      domain,
                      passwd,
                      &rpc_identity);
            if (serr)
            {
                goto error;
            }
        }
        rpc_binding_set_auth_info(rpc_binding_h,
            (unsigned char *) progArgs->spn,
            progArgs->protect_level,
            rpc_c_authn_gss_negotiate,
            rpc_identity,
            rpc_c_authz_name, &serr);
        if (serr)
        {
            goto error;
        }
    }


error:
    if (username)
    {
        free(username);
    }
    return serr;
}


void *ReverseItThread(void *in_ctx)
{
    threadarg *ctx = (threadarg *) in_ctx;
    unsigned int i = 0;
    int loop = ctx->progArgs->loop;
    int count = 0;
    args *outargs = NULL;
    unsigned32 status = 0;
    unsigned32 ok = 0;

    if (ctx->progArgs->silent < 2)
    {
        printf("ReverseItThread: called %p binding=%p\n", ctx, ctx->echo_server);
    }
    ctx->status = 0;
    while (loop)
    {
        count++;
        if (ctx->progArgs->silent < 2 && (count % 1000) == 0)
        {
            printf("ReverseItThread: %p Iteration=%d\n", ctx, count);
        }
        DO_RPC(ReverseIt(ctx->echo_server, ctx->inargs, &outargs, &status), ok);
        if (status || ok != 1)
        {
            ctx->status = status;
            printf("ReverseIt Failed status=%x\n", status);
            return (void *) 0;
        }
        else if (ctx->progArgs->silent == 0)
        {
            printf ("got response from server. results: \n");
            for (i=0; i<outargs->argc; i++)
                printf("\t[%u]: %s\n", i, outargs->argv[i]);
            printf("\n===================================\n");
        }
        freeOutargs(&outargs);
        loop--;
        if (ctx->progArgs->loop_sleep && loop)
        {
            sleep(ctx->progArgs->loop_sleep);
        }
    }
    return NULL;
}


int ReverseItInThreads(
    args *inargs,
    args **outargs,
    PROG_ARGS *progArgs,
    unsigned32 *status)
{
    dcethread **callers = NULL;
    threadarg *thread_arg_array = NULL;
    int bh_count = 0;
    int arg_cnt = 0;
    unsigned32 serr = 0;
    unsigned32 serr2 = 0;
    void *thread_status = NULL;
    rpc_binding_handle_t echo_server = NULL;

    callers = (dcethread **) calloc(progArgs->num_threads, sizeof(dcethread *));
    if (!callers)
    {
        serr = rpc_s_no_memory;
        goto error;
    }
    thread_arg_array = (threadarg *) calloc(progArgs->num_threads, sizeof(threadarg));
    if (!thread_arg_array)
    {
        serr = rpc_s_no_memory;
        goto error;
    }
    for (bh_count=0; bh_count<progArgs->rebind_count; bh_count++)
    {
        for (arg_cnt = 0; arg_cnt < progArgs->num_threads; arg_cnt++)
        {
            serr = create_binding_handle(&echo_server,
                                         echo_v1_0_c_ifspec,
                                         progArgs,
                                         bh_count);
            if (serr)
            {
                printf ("Couldnt obtain RPC server binding. exiting.\n");
                return(serr);
            }
            thread_arg_array[arg_cnt].echo_server = echo_server;
            echo_server = NULL;
            thread_arg_array[arg_cnt].inargs = inargs;
            thread_arg_array[arg_cnt].progArgs = progArgs;
        }

        for (arg_cnt=0; arg_cnt<progArgs->num_threads; arg_cnt++)
        {
            serr = dcethread_create(&callers[arg_cnt],
                                 NULL,
                                 ReverseItThread,
                                 &thread_arg_array[arg_cnt]);
            if (serr == -1)
            {
                printf("ReverseItInThreads: ERROR-dcethread_create() failed %d\n", errno);
                return 0;
            }
        }

        for (arg_cnt=0; arg_cnt<progArgs->num_threads; arg_cnt++)
        {
            dcethread_join(callers[arg_cnt], &thread_status);
            if (progArgs->silent < 2)
            {
                printf("joining threads: %d %p\n", arg_cnt, &thread_arg_array[arg_cnt]);
            }
            if (thread_status || thread_arg_array[arg_cnt].status)
            {
                printf("FATAL THREAD ERROR: ABORT!!!\n");
                exit(1);
            }
            if (progArgs->silent < 2)
            {
                printf("dcethread_join: %p\n", callers[arg_cnt]);
            }
        }

        /* force creation of new handle for all threads */
        for (arg_cnt=0; arg_cnt<progArgs->num_threads; arg_cnt++)
        {
            rpc_binding_free(&thread_arg_array[arg_cnt].echo_server, &serr2);
            thread_arg_array[arg_cnt].echo_server = NULL;
        }
    }
error:

    if (callers)
    {
        free((void *) callers);
    }
    if (thread_arg_array)
    {
        free(thread_arg_array);
    }
    if (serr)
    {
        *status = serr;
    }
    return 0;
}

static args *
alloc_inargs(PROG_ARGS progArgs)
{
    unsigned32 i = 0;
    char * nl = NULL;
    args * inargs = NULL;
    char buf[MAX_LINE+1];

    /*
     * Allocate an "args" struct with enough room to accomodate
     * the max number of lines of text we can can from stdin.
     */
    inargs = (args *)malloc(sizeof(args) + MAX_USER_INPUT * sizeof(string_t));
    if (inargs == NULL)
    {
        printf("FAULT. Didnt allocate inargs.\n");
        return NULL;
    }

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
    return inargs;
}

static unsigned32
create_binding_handle(
    rpc_binding_handle_t * ret_binding_handle,
    rpc_if_handle_t interface_spec,
    PROG_ARGS *progArgs,
    unsigned32 rebind)
{
    unsigned32 status = 0;
    unsigned32 good_bad_flags[] = {0, 1, 0, 1, 0, 1, 1, 0, 0, 0, 1, 1, 1, 0};
    unsigned32 gb_count = rebind;
    char *passwd = NULL;
    rpc_binding_handle_t binding_handle = NULL;

    /*
     * Get a binding handle to the server using the following params:
     *
     *  1. the hostname where the server lives
     *  2. the interface description structure of the IDL interface
     *  3. the desired transport protocol (UDP or TCP)
     */

    /* TBD:Adam-get_client_rpc_binding() can't fail, fix */
    status = get_client_rpc_binding(&binding_handle,
                               interface_spec,
                               progArgs->rpc_host,
                               progArgs->protocol,
                               progArgs->endpoint,
                               progArgs->silent);
    if (status != 1)
    {
        printf ("Couldnt obtain RPC server binding. exiting.\n");
        return(-1);
    }

    gb_count %= sizeof(good_bad_flags)/sizeof(good_bad_flags[0]);
    if (!good_bad_flags[gb_count] && progArgs->passwd_bad)
    {
        passwd = progArgs->passwd_bad;
        printf("create_rpc_identity: using bad password!!!\n");
    }
    else
    {
        passwd = progArgs->passwd;
        if (progArgs->passwd_bad)
        {
            /* Only print good/bad password use when both are specified */
            printf("create_rpc_identity: using good password :) \n");
        }
    }

    status = create_rpc_identity(progArgs, passwd, binding_handle);
    if (status)
    {
        return(-2);
    }
    *ret_binding_handle = binding_handle;
    return 0;
}

static unsigned32
do_rpc_call(
    PROG_ARGS *progArgs,
    args *inargs)
{
    threadarg thread_arg = {0};
    int rebind_count = 0;
    unsigned32 status = 0;
    rpc_binding_handle_t binding_handle = NULL;

    if (progArgs->silent < 2)
    {
        printf ("calling server\n");
    }
    for (rebind_count=0; rebind_count < progArgs->rebind_count; rebind_count++)
    {
        status = create_binding_handle(&binding_handle, echo_v1_0_c_ifspec, progArgs, rebind_count);
        switch (status)
        {
          case -1: printf ("Couldnt obtain RPC server binding. exiting.\n");
            return 1;
            break;
          case -2: printf ("Couldn't set auth info %u. exiting.\n", status);
            return 1;
            break;
        }

        if (status || !binding_handle)
        {
            printf("ERROR: do_rpc_call() returned error %x (%d)\n", status, status);
            continue;
        }

        thread_arg.echo_server = binding_handle;
        thread_arg.inargs = inargs;
        thread_arg.progArgs = progArgs;
        ReverseItThread(&thread_arg);

        status = thread_arg.status;
        if (status)
        {
            printf("ERROR: do_rpc_call() returned error %x (%d)\n", status, status);
            continue;
        }
        if (binding_handle)
        {
            rpc_binding_free(&binding_handle, &status);
        }

        if (status == error_status_ok &&
            progArgs->rebind_count > 0 &&
            progArgs->rebind_sleep > 0 &&
            (rebind_count+1) < progArgs->rebind_count)
        {
            sleep(progArgs->rebind_sleep);
        }
    }

    return thread_arg.status;
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
    ASSOC_ARGS assocs = {0};

    /*
     * stuff needed to make RPC calls
     */
    unsigned32 status = 0;
    args * outargs = NULL;
    int params = 0;
    int i = 0;
    char *envptr = NULL;

    argv0 = argv[0];

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
    status = parseArgs(argc, argv, &assocs, &params);
    if (status || assocs.cnt == 0 ||
        (!assocs.progArgs[0].rpc_host &&
         (strcmp(assocs.progArgs[0].protocol, PROTOCOL_NCALRPC) != 0 &&
          strcmp(assocs.progArgs[0].protocol, PROTOCOL_NP) != 0)))
    {
        fprintf(stderr, "parseArgs: failed %d\n", status);
        return 1;
    }

    for (i=0; i< assocs.cnt; i++)
    {
        /* TBD: move this into parseArgs ?? */
        assocs.progArgs[i].inargs = alloc_inargs(assocs.progArgs[i]);
        if (!assocs.progArgs[i].inargs)
        {
            return 1;
        }

        progArgs = assocs.progArgs[i];
        if (assocs.progArgs[i].do_threads)
        {
            status = ReverseItInThreads(progArgs.inargs, &outargs, &progArgs, &status);
            if (status)
            {
                printf("ERROR: ReverseItInThreads() returned error %x (%d)\n", status, status);
                goto cleanup;
            }
        }
        else
        {
            status = do_rpc_call(&progArgs, progArgs.inargs);
            if (status != error_status_ok && progArgs.rebind_count == 0)
            {
                printf("ERROR: do_rpc_call() returned error %x (%d)\n", status, status);
            }
        }
    }

cleanup:
    freeArgs(&progArgs);
    freeInargs(progArgs.inargs);
    freeOutargs(&outargs);
    return status ? 1 : 0;
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
    char * endpoint,
    int silent
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

    if (silent < 2)
    {
        printf("\n~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n"
               "fully resolving binding for server is: %s\n", string_binding);
    }

    rpc_string_free((unsigned char **) &string_binding, &status);
    chk_dce_err(status, "rpc_string_free()", "get_client_rpc_binding", 1);

    return 1;
}
