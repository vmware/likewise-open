#include <commonp.h>
#include <com.h>
#include <comprot.h>
#include <comnaf.h>
#include <comp.h>
#include <comsoc_ncalrpc.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <cnp.h>
#include <stddef.h>
#ifdef _WIN32

#ifndef CSIDL_COMMON_APPDATA

/* Defined in Shlobj.h, but pulls in rpcdce.h, so can't use here */
#define CSIDL_COMMON_APPDATA 0x0023
HRESULT
SHGetFolderPathA(
    HWND h,
    int type,
    HANDLE htok,
    DWORD flags,
    PSTR path);
#endif
#include <direct.h>
#ifndef strdup
#define strdup _strdup
#endif
#ifndef stat
#define stat _stat
#endif
#ifndef mkdir
#define mkdir(a, b) _mkdir((a))
#endif
#endif

#ifndef RPC_S_BUFSIZE
#define RPC_S_BUFSIZE RPC_C_CN_LARGE_FRAG_SIZE
#endif

static char *g_ncalrpc_basedir;
static int g_ncalrpc_basedir_len;

INTERNAL
char *_normalize_path_separator(char *path)
{
    char *ptr = NULL;

    for (ptr=path; *ptr; ptr++)
    {
        if (*ptr == '\\')
        {
            *ptr = '/';
        }
    }
    return path;
}

INTERNAL
unsigned32 _mkdir_path(char *dirpath)
{
    unsigned32 serr = 0;
    int sts = 0;
    char *tmppath = NULL;
    char *ptr = NULL;
    struct stat sb;

    memset(&sb, 0, sizeof(sb));
    tmppath = strdup(dirpath);
    if (!tmppath)
    {
        serr = rpc_s_no_memory;
    }

    _normalize_path_separator(tmppath);
    for (ptr = tmppath; *ptr; ptr++)
    {
        if (ptr != tmppath && *ptr == '/')
        {
            *ptr = '\0';
            sts = stat(tmppath, &sb);
            if (sts == -1 && errno == ENOENT && strlen(tmppath) != 2)
            {
                /*
                 * stat() may return ENOENT when the directory exists
                 * and access is denied, so we need to check for EEXIST
                 * from mkdir() here
                 */
                sts = mkdir(tmppath, 0755);
                if (sts == -1 && errno != EEXIST)
                {
                    serr = rpc_s_invalid_string_binding;
                    goto error;
                }
            }
            *ptr = '/';
        }
    }

error:
    if (tmppath)
    {
        free(tmppath);
    }
    return serr;
}

INTERNAL
rpc_socket_error_t
_rpc__ncalrpc_addr_allocate_port(
    rpc_protseq_id_t pseq_id,
    unsigned16 *port,
    rpc_addr_p_t *ret_addr)
{
    rpc_socket_error_t serr = RPC_C_SOCKET_OK;
    struct addrinfo *ai_addr = NULL;
    int ai_error = 0;
    rpc_ip_addr_p_t ip4addr = NULL;
    rpc_ip6_addr_p_t ip6addr = NULL;
    rpc_ip6_addr_p_t ipaddr_mem  = NULL;

    RPC_MEM_ALLOC(
        (rpc_addr_p_t) ipaddr_mem,
        rpc_addr_p_t,
        sizeof(rpc_ip6_addr_t),
        RPC_C_MEM_RPC_ADDR,
        RPC_C_MEM_WAITOK);
    if (!ipaddr_mem)
    {
        serr = rpc_s_no_memory;
        goto error;
    }

    if (pseq_id == RPC_C_PROTSEQ_ID_NCACN_IP6_TCP)
    {
        ip6addr = (rpc_ip6_addr_p_t) ipaddr_mem;
        ip6addr->rpc_protseq_id = RPC_C_PROTSEQ_ID_NCACN_IP6_TCP;
        ip6addr->len = sizeof(struct sockaddr_in6);
        ai_error = getaddrinfo((char *) RPC_NETWORK_IF_ADDR_LOOPBACK_IPV6, NULL, NULL, &ai_addr);
        if (ai_error)
        {
            serr = rpc_s_inval_net_addr;
            goto error;
        }
        ip6addr->sa = *(struct sockaddr_in6 *) ai_addr->ai_addr;
        if (port)
        {
            ip6addr->sa.sin6_port = htons(*port);
        }
    }
    else if (pseq_id == RPC_C_PROTSEQ_ID_NCACN_IP_TCP)
    {
        ip4addr = (rpc_ip_addr_p_t) ipaddr_mem;
        ip4addr->rpc_protseq_id = RPC_C_PROTSEQ_ID_NCACN_IP_TCP;
        ip4addr->len = sizeof(struct sockaddr_in);
        ai_error = getaddrinfo((char *) RPC_NETWORK_IF_ADDR_LOOPBACK_IPV4, NULL, NULL, &ai_addr);
        if (ai_error)
        {
            serr = rpc_s_inval_net_addr;
            goto error;
        }
        ip4addr->sa = *(struct sockaddr_in *) ai_addr->ai_addr;
        if (port)
        {
            ip4addr->sa.sin_port = htons(*port);
        }
    }
    else
    {
        serr = rpc_s_protseq_not_supported;
        goto error;
    }

    *ret_addr = (rpc_addr_p_t) ipaddr_mem;
error:
    if (serr)
    {
        if (ipaddr_mem)
        {
            free (ipaddr_mem);
        }
    }
    if (ai_addr)
    {
        freeaddrinfo(ai_addr);
    }
    return serr;
}

/*
 * Return the top directory of the ncalrpc endpoint "filesystem".
 * Note: This is called once, caching the result of the initial computation.
 * However, memory must be allocated, as the caller owns the return value.
 */
PRIVATE
rpc_socket_error_t
rpc__ncalrpc_get_endpoint_dirname(
    char **ret_dirname)
{
    rpc_socket_error_t serr = rpc_s_ok;
    int sts = 0;
    char *systemdata = NULL;
    char *systemdata_rpc = NULL;
    size_t systemdata_rpc_len = 0;
    char *dirname_path = NULL;
    char *basedir = NULL;
    size_t dirname_path_len = 0;
    struct stat sb;

    if (g_ncalrpc_basedir)
    {
        basedir = strdup(g_ncalrpc_basedir);
        if (!basedir)
        {
            serr = rpc_s_no_memory;
            goto error;
        }
        *ret_dirname = basedir;
        return RPC_C_SOCKET_OK;
    }

    basedir = getenv("VMWARE_TMP_DIR");
    if (!basedir)
    {
        /* Determine base directory to put ncalrpc endpoint */

        systemdata = calloc(sizeof(CHAR), MAX_PATH);
        if (!systemdata)
        {
            serr = rpc_s_no_memory;
            goto error;
        }
        if (SHGetFolderPathA(
                NULL,
                CSIDL_COMMON_APPDATA,
                NULL,
                0,
                systemdata) == 0)
        {
            systemdata_rpc_len = strlen(systemdata) +
                                 strlen(RPC_S_NCALRPC_ENDPOINT_VENDOR) + 1;
            systemdata_rpc = calloc(sizeof(CHAR), systemdata_rpc_len);
            if (!systemdata_rpc)
            {
                serr = rpc_s_no_memory;
                goto error;
            }

            /* Found "C:\ProgramData" or equivalent from system */
            snprintf(systemdata_rpc,
                     systemdata_rpc_len,
                     "%s%s",
                     systemdata,
                     RPC_S_NCALRPC_ENDPOINT_VENDOR);
            basedir = systemdata_rpc;
        }
        else
        {
            memset(&sb, 0, sizeof(sb));
            basedir = RPC_S_NCALRPC_ENDPOINT_DIR1;
            sts = stat(basedir, &sb);
            if (sts == -1)
            {
                memset(&sb, 0, sizeof(sb));
                basedir = RPC_S_NCALRPC_ENDPOINT_DIR2;
                sts = stat(basedir, &sb);
            }
            if (sts == -1)
            {
                serr = rpc_s_no_entry_name;
                goto error;
            }
        }
    }

    /* Allocate enough memory for full desired path */
    dirname_path_len = strlen(basedir) +
                         sizeof(RPC_S_NCALRPC_ENDPOINT_SUBDIR) + 1;
    dirname_path = calloc(dirname_path_len, sizeof(char));
    if (!dirname_path)
    {
        serr = rpc_s_no_memory;
        goto error;
    }

    /* Don't know if "/rpc" path exists, just create it */
    snprintf(dirname_path,
            dirname_path_len,
            "%s%s",
            basedir,
            RPC_S_NCALRPC_ENDPOINT_SUBDIR);
    serr = _mkdir_path(dirname_path);
    if (serr)
    {
        goto error;
    }

    /* Success, return the ncalrpc "filesystem" root */
    g_ncalrpc_basedir = strdup(dirname_path);
    if (!g_ncalrpc_basedir)
    {
        serr = rpc_s_no_memory;
        goto error;
    }
    *ret_dirname = dirname_path;

error:
    if (sts)
    {
        if (dirname_path)
        {
            free(dirname_path);
        }
    }
    if (systemdata)
    {
        free(systemdata);
    }
    if (systemdata_rpc)
    {
        free(systemdata_rpc);
    }
    return serr;
}

INTERNAL
rpc_socket_error_t
_rpc_ncalrpc_normalize_path(
    char *path,
    char **ret_normalized_path)
{
    rpc_socket_error_t serr = rpc_s_ok;
    char *normalized_path = NULL;
    unsigned32 len = 0;
    unsigned32 dwError = 0;
    unsigned32 i = 0;

    len = (unsigned32) strlen(path) + 1;
    normalized_path = (char *) calloc(len, sizeof(char));
    if (!normalized_path)
    {
        serr = rpc_s_no_memory;
        goto error;
    }

    dwError = GetFullPathNameA(path, len, normalized_path, NULL);
    if (dwError == 0)
    {
        RPC_DBG_PRINTF(rpc_e_dbg_general, RPC_C_CN_DBG_GENERAL,
            ("_rpc_ncalrpc_normalize_path: GetFullPathNameA: failed %u\n",
            GetLastError()));
        serr = rpc_s_invalid_string_binding;
        goto error;
    }
    for (i=0; i<len && normalized_path[i]; i++)
    {
        if (normalized_path[i] == '\\')
        {
            normalized_path[i] = '/';
        }
        else
        {
            normalized_path[i] = tolower(normalized_path[i]);
        }
    }
    RPC_DBG_PRINTF(rpc_e_dbg_general, RPC_C_CN_DBG_GENERAL,
        ("_rpc_ncalrpc_normalize_path: normalized_path=%s\n", normalized_path));
    *ret_normalized_path = normalized_path;

error:
    if (serr)
    {
        if (normalized_path)
        {
            free(normalized_path);
        }
    }

    return serr;
}


/*
 * Return a fully qualified path to the "endpoint". endpoint can
 * be a relative path containing subdirs. The fully qualified path
 * returned is rooted in the default ncalrpc endpoint "root" directory.
 * A fully qualified path passed in is merely returned to the caller.
 */
PRIVATE
rpc_socket_error_t
_rpc__ncalrpc_get_endpoint_filename(
    char *endpoint,
    char **ret_filename)
{
    rpc_socket_error_t serr = rpc_s_ok;
    int sts = 0;
    char *endpoint_path = NULL;
    char *basedir = NULL;
    size_t endpoint_path_len = 0;

    serr = rpc__ncalrpc_get_endpoint_dirname(&basedir);
    if (serr)
    {
        goto error;
    }

    /* Allocate enough memory for full desired path */
    endpoint_path_len = strlen(basedir) + strlen(endpoint) + 1;
    endpoint_path = calloc(endpoint_path_len, sizeof(char));
    if (!endpoint_path)
    {
        serr = rpc_s_no_memory;
        goto error;
    }

    /* Support fully qualified ncalrpc paths */
    if (RPC_NORMALIZE_SLASH(endpoint[0]) == '/' ||
        (isalpha(endpoint[0]) && endpoint[1] == ':' && RPC_NORMALIZE_SLASH(endpoint[2]) == '/'))
    {
        snprintf(endpoint_path,
                 endpoint_path_len,
                 "%s",
                 endpoint);
    }
    else
    {
        /*
         * Use ncalrpc "root", plus whatever path is provided for
         * the endpoint. Basedir has a trailing / in the path.
         */
        snprintf(endpoint_path,
                 endpoint_path_len,
                 "%s%s",
                 basedir,
                 endpoint);
    }
    RPC_DBG_PRINTF(rpc_e_dbg_general, RPC_C_CN_DBG_GENERAL,
                   ("endpoint file path=<%s>\n", endpoint_path));
    *ret_filename = endpoint_path;

error:
    if (serr)
    {
        if (endpoint_path)
        {
            free(endpoint_path);
        }
    }
    if (basedir)
    {
        free(basedir);
    }
    return serr;
}

PRIVATE
rpc_socket_error_t
rpc__ncalrpc_read_value_from_endpoint(
    char *endpoint,
    char **ret_value)
{
    rpc_socket_error_t serr = rpc_s_ok;
    FILE *fp = NULL;
    int sts = 0;
    char *new_str = NULL;
    char *new_endpoint = NULL;
    char *endpoint_path = NULL;
    char *cp = NULL;
    struct stat sb = {0};

    serr = _rpc__ncalrpc_get_endpoint_filename(endpoint, &endpoint_path);
    if (serr)
    {
        goto error;
    }

    sts = stat(endpoint_path, &sb);
    if (sts == -1)
    {
        serr = rpc_s_no_entry_name;
        goto error;
    }

    new_str = (char *) calloc(sb.st_size, sizeof(char));
    if (!new_str)
    {
        serr = rpc_s_no_memory;
        goto error;
    }

    fp = fopen(endpoint_path, "r");
    RPC_DBG_PRINTF(rpc_e_dbg_general, RPC_C_CN_DBG_GENERAL,
        ("rpc__ncalrpc_read_value_from_endpoint: fopen(%s)\n", endpoint_path));
    if (fp)
    {
        if (fgets(new_str, sb.st_size, fp))
        {
            cp = strrchr(new_str, '\r');
            if (cp)
            {
                *cp = '\0';
            }
            cp = strrchr(new_str, '\n');
            if (cp)
            {
                *cp = '\0';
            }
            new_endpoint = new_str;
            new_str = NULL;
            RPC_DBG_PRINTF(rpc_e_dbg_general, RPC_C_CN_DBG_GENERAL,
                ("rpc__ncalrpc_read_value_from_endpoint: new_endpoint=%s\n",
                new_endpoint));
        }
        fclose(fp);
    }
    else
    {
        /* Can't find better error for "unable to open file */
        serr = rpc_s_no_entry_name;
        goto error;
    }

    *ret_value = new_endpoint;
    new_str = NULL;

error:
    if (serr)
    {
        if (new_endpoint)
        {
            free(new_endpoint);
        }
    }
    if (endpoint_path)
    {
        free(endpoint_path);
    }
    if (new_str)
    {
        free(new_str);
    }
    return serr;
}

INTERNAL
rpc_socket_error_t
_rpc__ncalrpc_write_value_to_endpoint(
    char *endpoint,
    char *value)
{
    rpc_socket_error_t serr = rpc_s_ok;
    FILE *fp = NULL;
    int sts = 0;
    char *endpoint_path = NULL;

    serr = _rpc__ncalrpc_get_endpoint_filename(endpoint, &endpoint_path);
    if (serr)
    {
        goto error;
    }
    fp = fopen(endpoint_path, "w+");
    RPC_DBG_PRINTF(rpc_e_dbg_general, RPC_C_CN_DBG_GENERAL,
        ("_rpc__ncalrpc_write_value_to_endpoint: fopen(%s) endpoint=%d\n",
        endpoint_path, value));
    if (fp)
    {
        RPC_DBG_PRINTF(rpc_e_dbg_general, RPC_C_CN_DBG_GENERAL,
                       ("writing to file %s\n", endpoint_path));
        fprintf(fp, "%s\n", value);
        fclose(fp);
    }
    else
    {
        /* Can't find better error for "unable to open file */
        serr = rpc_s_no_memory;
        goto error;
    }

error:
    if (endpoint_path)
    {
        free(endpoint_path);
    }
    return serr;
}

INTERNAL
rpc_socket_error_t
rpc__ncalrpc_socket_construct(
    rpc_socket_t sock,
    rpc_protseq_id_t pseq_id ATTRIBUTE_UNUSED,
    rpc_transport_info_handle_t info
    )
{
    rpc_socket_error_t serr = RPC_C_SOCKET_OK;
    rpc_socket_t bsd_sock = NULL;
    unsigned short int family = 0;

    RPC_DBG_PRINTF(rpc_e_dbg_general, RPC_C_CN_DBG_GENERAL,
                   ("rpc__ncalrpc_socket_construct\n"));
    family = RPC_C_NAF_ID_IP;
    serr = rpc__socket_open(
               RPC_C_PROTSEQ_ID_NCACN_IP_TCP,
               NULL,
               &bsd_sock);
    if (serr != RPC_C_SOCKET_OK || !bsd_sock)
    {
        family = RPC_C_NAF_ID_IP6;
        serr = rpc__socket_open(
                   RPC_C_PROTSEQ_ID_NCACN_IP6_TCP,
                   NULL,
                   &bsd_sock);
    }
    sock->sock_transport = bsd_sock;
    return serr;
}

INTERNAL
rpc_socket_error_t
rpc__ncalrpc_socket_destruct(
    rpc_socket_t sock
    )
{
    rpc_socket_error_t serr = RPC_C_SOCKET_OK;
    char *service_name = NULL;
    char *port_path = NULL;
    rpc_socket_error_t serr2 = RPC_C_SOCKET_OK;
    rpc_socket_error_t serr3 = RPC_C_SOCKET_OK;

    RPC_DBG_PRINTF(rpc_e_dbg_general, RPC_C_CN_DBG_GENERAL,
                   ("rpc__ncalrpc_socket_destruct\n"));
    /* Sanity check the address type for NCALRPC */
    if (sock->pseq_id != RPC_C_PROTSEQ_ID_NCALRPC)
    {
        serr = rpc_s_prot_version_mismatch;
        goto error;
    }

    if (sock->ncalrpc_port)
    {
        serr2 = _rpc__ncalrpc_get_endpoint_filename(
                    sock->ncalrpc_port, &port_path);
        serr3 = rpc__ncalrpc_read_value_from_endpoint(
                    sock->ncalrpc_port, &service_name);
        if (serr2 == 0 && port_path)
        {
            remove(port_path);
        }
        if (serr3 == 0 && service_name)
        {
            remove(service_name);
        }
        free(sock->ncalrpc_port);
    }
    if (port_path)
    {
        free(port_path);
    }
    if (service_name)
    {
        free(service_name);
    }

    if (sock->sock_transport->data.pointer)
    {
        rpc__socket_close(sock->sock_transport);
    }

error:
    return serr;
}

INTERNAL
rpc_socket_error_t
rpc__ncalrpc_socket_bind(
    rpc_socket_t sock,
    rpc_addr_p_t addr
    )
{
    rpc_socket_error_t serr = RPC_C_SOCKET_OK;
    rpc_socket_error_t binderr = RPC_C_SOCKET_OK;
    rpc_addr_p_t localhost_addr = NULL;
    int bind_count = 0;
    int len = 0;
    char *cur_port_str = NULL;
    char *endpoint_filename = NULL;
    char *ncalrpc_endpoint = NULL;
    char *canon1 = NULL;
    char *canon2 = NULL;
    unsigned long port_ulong = 0;
    char *ep = NULL;
    unsigned16 ephemeral_port = RPC_S_NCALRPC_START_PORT;
    boolean endpoint_exists = false;
    char str_port[8];

    RPC_DBG_PRINTF(rpc_e_dbg_general, RPC_C_CN_DBG_GENERAL,
                   ("rpc__ncalrpc_socket_bind called\n"));

    /*
     * Test for client/server side call. Client doesn't have
     * address populated, but the server does.
     */
    if (addr->rpc_protseq_id == RPC_C_PROTSEQ_ID_NCALRPC &&
        strlen((char *) addr->sa.data) == 0)
    {
        goto error;
    }

    serr = _rpc__ncalrpc_addr_allocate_port(
               sock->sock_transport->pseq_id,
               NULL,
               &localhost_addr);
    if (serr)
    {
        goto error;
    }

    /* Save off "AF_UNIX" binding before it is destroyed in the next call */
    snprintf(sock->endpoint, sizeof(sock->endpoint), "%s", addr->sa.data);

    ncalrpc_endpoint = strdup(sock->endpoint);
    if (!ncalrpc_endpoint)
    {
        serr = rpc_s_no_memory;
        goto error;
    }

    /* Determine if existing port is already in use */
    serr = rpc__ncalrpc_read_value_from_endpoint(
               ncalrpc_endpoint,
               &cur_port_str);
    if (serr == rpc_s_ok)
    {
        endpoint_exists = true;
        port_ulong = strtoul(cur_port_str, &ep, 10);
        if (ep && *ep != '\0')
        {
            serr = rpc_s_invalid_endpoint_format;
            goto error;
        }

        /*
         * Validate input, in case someone created "bogus" endpoint file entry.
         */
        if (port_ulong > (RPC_S_NCALRPC_START_PORT+RPC_S_NCALRPC_MAX_PORT_RANGE) ||
            port_ulong < RPC_S_NCALRPC_START_PORT)
        {
            ephemeral_port = RPC_S_NCALRPC_START_PORT;
        }
        else
        {
            ephemeral_port = (unsigned16) port_ulong;
        }
    }

    /*
     * Start searching for an available ephemeral port. Should an endpoint file
     * already exist, the search begins with the value from that file. Otherwise,
     * the search begins at the top of the ncalrpc port range. The port is deemed
     * "in use" should the bind operation fail. Bind failure is a special
     * case when the port was initially obtained from the endpoint file. The
     * port file corresponding to this endpoint is inspected, and if the
     * same endpoint name is in the port file, then this endpoint is already
     * in use, and an error is returned. Otherwise, the port is merely in use
     * by an unrelated service, and the search for a free port, starting at the
     * top of the port range begins.
     */
    do
    {
        if (sock->sock_transport->pseq_id == RPC_C_PROTSEQ_ID_NCACN_IP6_TCP)
        {
            ((rpc_ip6_addr_p_t) localhost_addr)->sa.sin6_port = htons(ephemeral_port);
        }
        else
        {
            ((rpc_ip_addr_p_t) localhost_addr)->sa.sin_port = htons(ephemeral_port);
        }
        binderr = rpc__socket_bind (
                      sock->sock_transport,
                      localhost_addr);
        if (binderr == rpc_s_ok)
        {
            snprintf(str_port, sizeof(str_port), "%d", ephemeral_port);

            /* Save port path in socket for later cleanup */
            sock->ncalrpc_port = strdup(str_port);
            if (!sock->ncalrpc_port)
            {
                serr = rpc_s_no_memory;
                goto error;
            }

            /*
             * State: Successfully bound port to socket. Determine which
             * endpoint / port mapping files must be updated.
             * Must create initially if endpoint_exists is false.
             */
            if (!endpoint_exists)
            {
                /* Endpoint->port association */
                serr = _rpc__ncalrpc_write_value_to_endpoint(
                           ncalrpc_endpoint, str_port);
                if (serr)
                {
                    goto error;
                }

                serr = _rpc__ncalrpc_write_value_to_endpoint(
                           str_port, ncalrpc_endpoint);
                if (serr)
                {
                    goto error;
                }
            }
            else
            {
                /* Fix port file if endpoint value doesn't match */
                serr = rpc__ncalrpc_read_value_from_endpoint(
                           cur_port_str, &endpoint_filename);
                if (serr)
                {
                    /*
                     * Inconsistent state. Endpoint exists but not the port
                     * file, so create the port file.
                     */
                    serr = _rpc__ncalrpc_write_value_to_endpoint(
                           str_port, ncalrpc_endpoint);
                    if (serr)
                    {
                        goto error;
                    }
                    serr = rpc__ncalrpc_read_value_from_endpoint(
                               cur_port_str, &endpoint_filename);
                }
                if (serr)
                {
                    goto error;
                }

                if (!endpoint_filename)
                {
                    serr = rpc_s_invalid_endpoint_format;
                    goto error;
                }

                serr = _rpc_ncalrpc_normalize_path(endpoint_filename, &canon1);
                if (serr)
                {
                    goto error;
                }
                serr = _rpc_ncalrpc_normalize_path(ncalrpc_endpoint, &canon2);
                if (serr)
                {
                    goto error;
                }
                RPC_DBG_PRINTF(rpc_e_dbg_general, RPC_C_CN_DBG_GENERAL,
                    ("rpc__ncalrpc_socket_bind: strcmp/1 canon1=%s canon2=%s\n",
                    canon1, canon2));
                if (strcmp(canon1, canon2) != 0)
                {
                    serr = _rpc__ncalrpc_write_value_to_endpoint(
                               str_port, ncalrpc_endpoint);
                    if (serr)
                    {
                        goto error;
                    }
                }
            }
        }
        else if (endpoint_exists)
        {
            /* Validate endpoint (port) available */
            snprintf(str_port, sizeof(str_port), "%d", ephemeral_port);
            serr = rpc__ncalrpc_read_value_from_endpoint(
                       str_port, &endpoint_filename);
            if (serr)
            {
                goto error;
            }
            serr = _rpc_ncalrpc_normalize_path(endpoint_filename, &canon1);
            if (serr)
            {
                goto error;
            }
            serr = _rpc_ncalrpc_normalize_path(ncalrpc_endpoint, &canon2);
            if (serr)
            {
                goto error;
            }
            RPC_DBG_PRINTF(rpc_e_dbg_general, RPC_C_CN_DBG_GENERAL,
                ("rpc__ncalrpc_socket_bind: strcmp/2 canon1=%s canon2=%s\n",
                canon1, canon2));
            if (strcmp(canon1, canon2) == 0)
            {
                /*
                 * endpoint name corresponds to a valid endpoint number
                 * that is in use (endpoint file and port file reference
                 * each other). In this case, the endpoint is determined
                 * to be in use, so return an error.
                 */
                serr = rpc_s_cant_bind_socket;
                goto error;
            }
            ephemeral_port = RPC_S_NCALRPC_START_PORT;
            endpoint_exists = false;
        }
        else
        {
            RPC_DBG_PRINTF(rpc_e_dbg_general, RPC_C_CN_DBG_GENERAL,
                ("rpc__ncalrpc_socket_bind: failed %d port=%d\n",
                serr, ephemeral_port));
            ephemeral_port++;
            bind_count++;
        }
        if (ephemeral_port >= (RPC_S_NCALRPC_START_PORT+RPC_S_NCALRPC_MAX_PORT_RANGE))
        {
            ephemeral_port = RPC_S_NCALRPC_START_PORT;
        }
    }
    while ((binderr == WSAEADDRINUSE ||
            binderr == WSAEACCES ||
            binderr == WSAEADDRNOTAVAIL) &&
           bind_count < RPC_S_NCALRPC_MAX_PORT_RANGE);

    if (bind_count >= RPC_S_NCALRPC_MAX_PORT_RANGE)
    {
        RPC_DBG_PRINTF(rpc_e_dbg_general, RPC_C_CN_DBG_GENERAL,
            ("rpc__ncalrpc_socket_bind: no available ports error=%d port=%d\n",
            serr, ephemeral_port));
    }

error:
    if (ncalrpc_endpoint)
    {
        free(ncalrpc_endpoint);
    }
    if (endpoint_filename)
    {
        free(endpoint_filename);
    }
    if (cur_port_str)
    {
        free(cur_port_str);
    }
    if (localhost_addr)
    {
        free (localhost_addr);
    }
    if (canon2)
    {
        free(canon2);
    }
    if (canon1)
    {
        free(canon1);
    }

    return serr;
}

INTERNAL
rpc_socket_error_t
rpc__ncalrpc_socket_connect(
    rpc_socket_t sock,
    rpc_addr_p_t addr,
    rpc_cn_assoc_t *assoc ATTRIBUTE_UNUSED
    )
{
    rpc_socket_error_t serr = RPC_C_SOCKET_OK;
    rpc_addr_p_t localhost_addr = NULL;
    char *ep = NULL;
    char *port_str = NULL;
    char *endpoint_str = NULL;
    char *endpoint_str_canon = NULL;
    unsigned16 port = 0;

    RPC_DBG_PRINTF(rpc_e_dbg_general, RPC_C_CN_DBG_GENERAL,
                   ("rpc__ncalrpc_socket_connect\n"));


    /* Sanity check the address type for NCALRPC */
    if (sock->pseq_id != RPC_C_PROTSEQ_ID_NCALRPC &&
        addr->sa.family != RPC_C_NAF_ID_NCALRPC)
    {
        serr = rpc_s_prot_version_mismatch;
        goto error;
    }

    /* Get endpoint name from address structure */
    serr = _rpc_ncalrpc_normalize_path(
                addr->sa.data,
                &ep);
    if (serr)
    {
        goto error;
    }

    /* Find corresponding server port for specified endpoint */
    serr = rpc__ncalrpc_read_value_from_endpoint(ep, &port_str);
    if (serr)
    {
        serr = rpc_s_endpoint_not_found;
        goto error;
    }

    /* Reverse lookup the port to verify the endpoint matches */
    serr = rpc__ncalrpc_read_value_from_endpoint(port_str, &endpoint_str);
    if (serr)
    {
        serr = rpc_s_no_port;
        goto error;
    }
    serr = _rpc_ncalrpc_normalize_path(endpoint_str, &endpoint_str_canon);
    if (serr)
    {
        serr = rpc_s_no_port;
        goto error;
    }
    if (strcmp(ep, endpoint_str_canon) != 0)
    {
        serr = rpc_s_endpoint_not_found;
        goto error;
    }

    port = (unsigned16) atoi(port_str);
    serr = _rpc__ncalrpc_addr_allocate_port(
               sock->sock_transport->pseq_id,
               &port,
               &localhost_addr);
    if (serr)
    {
        goto error;
    }

    serr = rpc__socket_connect(
               sock->sock_transport,
               localhost_addr,
               NULL);
    if (serr)
    {
        goto error;
    }

error:
    if (port_str)
    {
        free(port_str);
    }
    if (localhost_addr)
    {
        free (localhost_addr);
    }
    if (ep)
    {
        free(ep);
    }
    if (endpoint_str_canon)
    {
        free(endpoint_str_canon);
    }
    if (endpoint_str)
    {
        free(endpoint_str);
    }
    return serr;
}


INTERNAL
rpc_socket_error_t
rpc__ncalrpc_socket_accept(
    rpc_socket_t sock,
    rpc_addr_p_t addr,
    rpc_socket_t *newsock
    )
{
    rpc_socket_error_t serr = RPC_C_SOCKET_OK;

    RPC_DBG_PRINTF(rpc_e_dbg_general, RPC_C_CN_DBG_GENERAL,
                   ("rpc__ncalrpc_socket_accept\n"));
    serr = rpc__socket_accept (
               sock->sock_transport,
               addr,
               newsock);
    return serr;
}

INTERNAL
rpc_socket_error_t
rpc__ncalrpc_socket_listen(
    rpc_socket_t sock,
    int backlog
    )
{
    rpc_socket_error_t serr = RPC_C_SOCKET_OK;

    RPC_DBG_PRINTF(rpc_e_dbg_general, RPC_C_CN_DBG_GENERAL,
                   ("rpc__ncalrpc_socket_listen\n"));
    serr = rpc__socket_listen (
               sock->sock_transport,
               backlog);
    return serr;
}

INTERNAL
rpc_socket_error_t
rpc__ncalrpc_socket_sendmsg(
    rpc_socket_t sock,
    rpc_socket_iovec_p_t iov,
    int iov_len,
    rpc_addr_p_t addr ATTRIBUTE_UNUSED,
    int *cc
    )
{
    rpc_socket_error_t serr = RPC_C_SOCKET_OK;

    RPC_DBG_PRINTF(rpc_e_dbg_general, RPC_C_CN_DBG_GENERAL,
                   ("rpc__ncalrpc_socket_sendmsg\n"));
    /* Sanity check the address type for NCALRPC */
    if (sock->pseq_id != RPC_C_PROTSEQ_ID_NCALRPC &&
        addr->sa.family != RPC_C_NAF_ID_NCALRPC)
    {
        serr = rpc_s_prot_version_mismatch;
        goto error;
    }

    serr = rpc__socket_sendmsg(
               sock->sock_transport,
               iov,
               iov_len,
               addr,
               cc);
    if (serr)
    {
        goto error;
    }


error:
    return serr;
}

INTERNAL
rpc_socket_error_t
rpc__ncalrpc_socket_recvmsg(
    rpc_socket_t sock,
    rpc_socket_iovec_p_t iov,
    int iov_len,
    rpc_addr_p_t addr,
    int *cc
)
{
    rpc_socket_error_t serr = RPC_C_SOCKET_OK;

    RPC_DBG_PRINTF(rpc_e_dbg_general, RPC_C_CN_DBG_GENERAL,
                   ("rpc__ncalrpc_socket_recvmsg\n"));
    /* Sanity check the address type for NCALRPC */
    if (sock->pseq_id != RPC_C_PROTSEQ_ID_NCALRPC &&
        addr->sa.family != RPC_C_NAF_ID_NCALRPC)
    {
        serr = rpc_s_prot_version_mismatch;
        goto error;
    }

    serr = rpc__socket_recvmsg(
               sock->sock_transport,
               iov,
               iov_len,
               addr,
               cc);
    if (serr)
    {
        goto error;
    }

error:
    return serr;
}

INTERNAL
rpc_socket_error_t
rpc__ncalrpc_socket_recvfrom(
    rpc_socket_t sock ATTRIBUTE_UNUSED,
    byte_p_t buf ATTRIBUTE_UNUSED,
    int len ATTRIBUTE_UNUSED,
    rpc_addr_p_t from ATTRIBUTE_UNUSED,
    int *cc ATTRIBUTE_UNUSED
)
{
    rpc_socket_error_t serr = ENOTSUP;


    RPC_DBG_PRINTF(rpc_e_dbg_general, RPC_C_CN_DBG_GENERAL,
                   ("rpc__ncalrpc_socket_recvfrom\n"));
    return serr;
}


INTERNAL
rpc_socket_error_t
rpc__ncalrpc_socket_inq_endpoint(
    rpc_socket_t sock,
    rpc_addr_p_t addr
)
{
    rpc_socket_error_t serr = RPC_C_SOCKET_OK;
    RPC_DBG_PRINTF(rpc_e_dbg_general, RPC_C_CN_DBG_GENERAL,
                   ("rpc__ncalrpc_socket_inq_endpoint\n"));
    return serr;
}

INTERNAL
rpc_socket_error_t
rpc__ncalrpc_socket_set_broadcast(
    rpc_socket_t sock ATTRIBUTE_UNUSED
)
{
    rpc_socket_error_t serr = RPC_C_SOCKET_OK;

    RPC_DBG_PRINTF(rpc_e_dbg_general, RPC_C_CN_DBG_GENERAL,
                   ("rpc__ncalrpc_socket_set_broadcast\n"));
    return serr;
}

INTERNAL
rpc_socket_error_t
rpc__ncalrpc_socket_set_bufs(
    rpc_socket_t sock ATTRIBUTE_UNUSED,
    unsigned32 txsize ATTRIBUTE_UNUSED,
    unsigned32 rxsize ATTRIBUTE_UNUSED,
    unsigned32 *ntxsize ATTRIBUTE_UNUSED,
    unsigned32 *nrxsize ATTRIBUTE_UNUSED
)
{
    rpc_socket_error_t serr = RPC_C_SOCKET_OK;

    RPC_DBG_PRINTF(rpc_e_dbg_general, RPC_C_CN_DBG_GENERAL,
                   ("rpc__ncalrpc_socket_set_bufs\n"));
    return serr;
}

INTERNAL
rpc_socket_error_t
rpc__ncalrpc_socket_set_nbio(
    rpc_socket_t sock ATTRIBUTE_UNUSED
    )
{
    rpc_socket_error_t serr = ENOTSUP;


    RPC_DBG_PRINTF(rpc_e_dbg_general, RPC_C_CN_DBG_GENERAL,
                   ("rpc__ncalrpc_socket_set_nbio\n"));
    return serr;
}

INTERNAL
rpc_socket_error_t
rpc__ncalrpc_socket_set_close_on_exec(
    rpc_socket_t sock ATTRIBUTE_UNUSED
    )
{
    rpc_socket_error_t serr = RPC_C_SOCKET_OK;

    RPC_DBG_PRINTF(rpc_e_dbg_general, RPC_C_CN_DBG_GENERAL,
                   ("rpc__ncalrpc_socket_set_close_on_exec\n"));
    return serr;
}

INTERNAL
rpc_socket_error_t
rpc__ncalrpc_socket_getpeername(
    rpc_socket_t sock,
    rpc_addr_p_t addr
    )
{
    rpc_socket_error_t serr = RPC_C_SOCKET_OK;

    RPC_DBG_PRINTF(rpc_e_dbg_general, RPC_C_CN_DBG_GENERAL,
                   ("rpc__ncalrpc_socket_getpeername\n"));
    return serr;
}

INTERNAL
rpc_socket_error_t
rpc__ncalrpc_socket_get_if_id(
    rpc_socket_t sock ATTRIBUTE_UNUSED,
    rpc_network_if_id_t *network_if_id ATTRIBUTE_UNUSED
    )
{
    rpc_socket_error_t serr = RPC_C_SOCKET_OK;

    *network_if_id = SOCK_STREAM;

    RPC_DBG_PRINTF(rpc_e_dbg_general, RPC_C_CN_DBG_GENERAL,
                   ("rpc__ncalrpc_socket_get_if_id\n"));
    return serr;
}

INTERNAL
rpc_socket_error_t
rpc__ncalrpc_socket_set_keepalive(
    rpc_socket_t sock ATTRIBUTE_UNUSED
    )
{
    rpc_socket_error_t serr = RPC_C_SOCKET_OK;

    RPC_DBG_PRINTF(rpc_e_dbg_general, RPC_C_CN_DBG_GENERAL,
                   ("rpc__ncalrpc_socket_set_keepalive\n"));
    return serr;
}

INTERNAL
rpc_socket_error_t
rpc__ncalrpc_socket_nowriteblock_wait(
    rpc_socket_t sock ATTRIBUTE_UNUSED,
    struct timeval *tmo ATTRIBUTE_UNUSED
    )
{
    rpc_socket_error_t serr = ENOTSUP;


    RPC_DBG_PRINTF(rpc_e_dbg_general, RPC_C_CN_DBG_GENERAL,
                   ("rpc__ncalrpc_socket_nowriteblock_wait\n"));
    return serr;
}

INTERNAL
rpc_socket_error_t
rpc__ncalrpc_socket_set_rcvtimeo(
    rpc_socket_t sock ATTRIBUTE_UNUSED,
    struct timeval *tmo ATTRIBUTE_UNUSED
    )
{
    rpc_socket_error_t serr = RPC_C_SOCKET_OK;

    RPC_DBG_PRINTF(rpc_e_dbg_general, RPC_C_CN_DBG_GENERAL,
                   ("rpc__ncalrpc_socket_set_rcvtimeo\n"));
    return serr;
}

INTERNAL
rpc_socket_error_t
rpc__ncalrpc_socket_getpeereid(
    rpc_socket_t sock ATTRIBUTE_UNUSED,
    uid_t *euid ATTRIBUTE_UNUSED,
    gid_t *egid ATTRIBUTE_UNUSED
    )
{
    rpc_socket_error_t serr = ENOTSUP;


    RPC_DBG_PRINTF(rpc_e_dbg_general, RPC_C_CN_DBG_GENERAL,
                   ("rpc__ncalrpc_socket_getpeereid\n"));
    return serr;
}

INTERNAL
int
rpc__ncalrpc_socket_get_select_desc(
    rpc_socket_t sock
    )
{
    RPC_DBG_PRINTF(rpc_e_dbg_general, RPC_C_CN_DBG_GENERAL,
                   ("rpc__ncalrpc_socket_get_select_desc\n"));
    return rpc__socket_get_select_desc(sock->sock_transport);
}

INTERNAL
rpc_socket_error_t
rpc__ncalrpc_socket_enum_ifaces(
    rpc_socket_t sock ATTRIBUTE_UNUSED,
    rpc_socket_enum_iface_fn_p_t efun ATTRIBUTE_UNUSED,
    rpc_addr_vector_p_t *rpc_addr_vec ATTRIBUTE_UNUSED,
    rpc_addr_vector_p_t *netmask_addr_vec ATTRIBUTE_UNUSED,
    rpc_addr_vector_p_t *broadcast_addr_vec ATTRIBUTE_UNUSED
    )
{
    rpc_socket_error_t serr = ENOTSUP;


    RPC_DBG_PRINTF(rpc_e_dbg_general, RPC_C_CN_DBG_GENERAL,
                   ("rpc__ncalrpc_socket_enum_ifaces\n"));
    return serr;
}

INTERNAL
rpc_socket_error_t
rpc__ncalrpc_socket_inq_transport_info(
    rpc_socket_t sock,
    rpc_transport_info_handle_t* info
    )
{
    rpc_socket_error_t serr = RPC_C_SOCKET_OK;

    RPC_DBG_PRINTF(rpc_e_dbg_general, RPC_C_CN_DBG_GENERAL,
                   ("rpc__ncalrpc_socket_inq_transport_info\n"));
    return serr;
}

INTERNAL
rpc_socket_error_t
rpc__ncalrpc_socket_transport_inq_access_token(
    rpc_transport_info_handle_t info,
    rpc_access_token_p_t* token
    )
{
    *token = NULL;

    RPC_DBG_PRINTF(rpc_e_dbg_general, RPC_C_CN_DBG_GENERAL,
                   ("rpc__ncalrpc_socket_transport_inq_access_token\n"));
    return 0;
}

INTERNAL
void
rpc__ncalrpc_transport_info_free(
    rpc_transport_info_handle_t info
    )
{
}

INTERNAL
boolean
rpc__ncalrpc_transport_info_equal(
    rpc_transport_info_handle_t info1,
    rpc_transport_info_handle_t info2
    )
{
    rpc_socket_error_t serr = RPC_C_SOCKET_OK;

    RPC_DBG_PRINTF(rpc_e_dbg_general, RPC_C_CN_DBG_GENERAL,
                   ("rpc__ncalrpc_transport_info_equal\n"));
    return serr;
}

INTERNAL
rpc_socket_error_t
rpc__ncalrpc_transport_inq_access_token(
    rpc_transport_info_handle_t info,
    rpc_access_token_p_t* token
    )
{
    rpc_socket_error_t serr = RPC_C_SOCKET_OK;

    RPC_DBG_PRINTF(rpc_e_dbg_general, RPC_C_CN_DBG_GENERAL,
                   ("rpc__ncalrpc_transport_inq_access_token\n"));
    return serr;
}

rpc_socket_vtbl_t rpc_g_ncalrpc_socket_vtbl =
{
    rpc__ncalrpc_socket_construct,
    rpc__ncalrpc_socket_destruct,
    rpc__ncalrpc_socket_bind,
    rpc__ncalrpc_socket_connect,
    rpc__ncalrpc_socket_accept,
    rpc__ncalrpc_socket_listen,
    rpc__ncalrpc_socket_sendmsg,
    rpc__ncalrpc_socket_recvfrom,
    rpc__ncalrpc_socket_recvmsg,
    rpc__ncalrpc_socket_inq_endpoint,
    rpc__ncalrpc_socket_set_broadcast,
    rpc__ncalrpc_socket_set_bufs,
    rpc__ncalrpc_socket_set_nbio,
    rpc__ncalrpc_socket_set_close_on_exec,
    rpc__ncalrpc_socket_getpeername,
    rpc__ncalrpc_socket_get_if_id,
    rpc__ncalrpc_socket_set_keepalive,
    rpc__ncalrpc_socket_nowriteblock_wait,
    rpc__ncalrpc_socket_set_rcvtimeo,
    rpc__ncalrpc_socket_getpeereid,
    rpc__ncalrpc_socket_get_select_desc,
    rpc__ncalrpc_socket_enum_ifaces,
    rpc__ncalrpc_socket_inq_transport_info,
    rpc__ncalrpc_transport_info_free,
    rpc__ncalrpc_transport_info_equal,
    rpc__ncalrpc_transport_inq_access_token
};
