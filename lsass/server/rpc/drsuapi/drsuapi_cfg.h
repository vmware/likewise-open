#ifndef _DRSUAPI_CFG_H_
#define _DRSUAPI_CFG_H_


typedef struct drsuapi_srv_config
{
    PSTR pszLpcSocketPath;
    PSTR pszDefaultLoginShell;
    PSTR pszHomedirPrefix;
    PSTR pszHomedirTemplate;
    BOOLEAN bRegisterTcpIp;
} DRSUAPI_SRV_CONFIG, *PDRSUAPI_SRV_CONFIG;


DWORD
DrsuapiSrvInitialiseConfig(
    PDRSUAPI_SRV_CONFIG pConfig
    );


VOID
DrsuapiSrvFreeConfigContents(
    PDRSUAPI_SRV_CONFIG pConfig
    );


DWORD
DrsuapiSrvReadRegistry(
    PDRSUAPI_SRV_CONFIG pConfig
    );


DWORD
DrsuapiSrvConfigGetLpcSocketPath(
    PSTR *ppszLpcSocketPath
    );


DWORD
DrsuapiSrvConfigGetDefaultLoginShell(
    PSTR *ppszDefaultLoginShell
    );


DWORD
DrsuapiSrvConfigGetHomedirPrefix(
    PSTR *ppszLpcSocketPath
    );


DWORD
DrsuapiSrvConfigGetHomedirTemplate(
    PSTR *ppszHomedirTemplate
    );

DWORD
DrsuapiSrvConfigShouldRegisterTcpIp( BOOLEAN* pbResult
    );

#endif /* _DRSUAPI_CFG_H_ */


/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
