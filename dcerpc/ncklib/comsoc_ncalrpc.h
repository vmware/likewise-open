#ifndef _COMSOC_NCALRPC_H
#define _COMSOC_NCALRPC_H

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <dce/dce.h>

#include <comsoc.h>

EXTERNAL rpc_socket_vtbl_t rpc_g_ncalrpc_socket_vtbl;

/* Start search for ephemeral port here */
#define RPC_S_NCALRPC_START_PORT 50000
#define RPC_S_NCALRPC_MAX_PORT_RANGE 1024
#define RPC_S_NCALRPC_ENDPOINT_DIR1 "c:/ProgramData/VMware/vCenterServer"
#define RPC_S_NCALRPC_ENDPOINT_DIR2 "c:/Documents and Settings/All Users/Application Data"
#define RPC_S_NCALRPC_ENDPOINT_SUBDIR "/rpc/"
#ifndef RPC_S_NCALRPC_ENDPOINT_VENDOR
#define RPC_S_NCALRPC_ENDPOINT_VENDOR "/VMware/vCenterServer"
#endif

PRIVATE
rpc_socket_error_t
rpc__ncalrpc_read_value_from_endpoint(
    char *endpoint,
    char **ret_value);

PRIVATE
rpc_socket_error_t
rpc__ncalrpc_get_endpoint_dirname(
    char **ret_dirname);

PRIVATE
rpc_socket_error_t
_rpc__ncalrpc_get_endpoint_filename(
    char *endpoint,
    char **ret_filename);

#endif
/*
 * Return a fully qualified path to the "endpoint". endpoint can
 * be a relative path containing subdirs. The fully qualified path
 * returned is rooted in the default ncalrpc endpoint "root" directory.
 * A fully qualified path passed in is merely returned to the caller.
 */
