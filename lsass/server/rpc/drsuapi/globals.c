#include "includes.h"

/* Library initialisation guard */
pthread_mutex_t gDrsuapiSrvDataMutex = PTHREAD_MUTEX_INITIALIZER;

int bDrsuapiSrvInitialised = 0;

PCSTR gpszDrsuapiRpcSrvName = "netlogon";
LSA_RPCSRV_FUNCTION_TABLE gDrsuapiRpcFuncTable = {
    &DrsuapiRpcStartServer,
    &DrsuapiRpcStopServer
};

rpc_binding_vector_p_t gpDrsuapiSrvBinding = NULL;

DRSUAPI_SRV_CONFIG gDrsuapiSrvConfig;

