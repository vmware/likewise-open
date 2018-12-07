/*
 * Copyright (C) VMware. All rights reserved.
 */

#include "includes.h"

PCSTR gpszVmDirProviderName = LSA_PROVIDER_TAG_VMDIR;

LSA_PROVIDER_FUNCTION_TABLE gVmDirProviderAPITable =
{
    .pfnFindObjects             = &VmDirFindObjects,
    .pfnOpenEnumObjects         = &VmDirOpenEnumObjects,
    .pfnEnumObjects             = &VmDirEnumObjects,
    .pfnOpenEnumGroupMembers    = &VmDirOpenEnumMembers,
    .pfnEnumGroupMembers        = &VmDirEnumMembers,
    .pfnCloseEnum               = &VmDirCloseEnum,
    .pfnQueryMemberOf           = &VmDirQueryMemberOf,
    .pfnGetSmartCardUserObject  = &VmDirGetSmartCardUserObject,
    .pfnGetMachineAccountInfoA  = &VmDirGetMachineAccountInfoA,
    .pfnGetMachineAccountInfoW  = &VmDirGetMachineAccountInfoW,
    .pfnGetMachinePasswordInfoA = &VmDirGetMachinePasswordInfoA,
    .pfnGetMachinePasswordInfoW = &VmDirGetMachinePasswordInfoW,
    .pfnShutdownProvider        = &VmDirShutdownProvider,
    .pfnOpenHandle              = &VmDirOpenHandle,
    .pfnCloseHandle             = &VmDirCloseHandle,
    .pfnServicesDomain          = &VmDirServicesDomain,
    .pfnAuthenticateUserPam     = &VmDirAuthenticateUserPam,
    .pfnAuthenticateUserEx      = &VmDirAuthenticateUserEx,
    .pfnValidateUser            = &VmDirValidateUser,
    .pfnCheckUserInList         = &VmDirCheckUserInList,
    .pfnChangePassword          = &VmDirChangePassword,
    .pfnSetPassword             = &VmDirSetPassword,
    .pfnAddUser                 = &VmDirAddUser,
    .pfnModifyUser              = &VmDirModifyUser,
    .pfnAddGroup                = &VmDirAddGroup,
    .pfnModifyGroup             = &VmDirModifyGroup,
    .pfnDeleteObject            = &VmDirDeleteObject,
    .pfnOpenSession             = &VmDirOpenSession,
    .pfnCloseSession            = &VmDirCloseSession,
    .pfnLookupNSSArtefactByKey  = &VmDirFindNSSArtefactByKey,
    .pfnBeginEnumNSSArtefacts   = &VmDirBeginEnumNSSArtefacts,
    .pfnEnumNSSArtefacts        = &VmDirEnumNSSArtefacts,
    .pfnEndEnumNSSArtefacts     = &VmDirEndEnumNSSArtefacts,
    .pfnGetStatus               = &VmDirGetStatus,
    .pfnFreeStatus              = &VmDirFreeStatus,
    .pfnRefreshConfiguration    = &VmDirRefreshConfiguration,
    .pfnProviderIoControl       = &VmDirProviderIoControl
};

VMDIR_AUTH_PROVIDER_GLOBALS gVmDirAuthProviderGlobals =
{
    .pMutex_rw  = NULL,
    .pMutex     = NULL,
    .pBindInfo  = NULL,
    .joinState  = VMDIR_JOIN_STATE_UNSET,
    .pRefreshContext = NULL,
    .bindProtocol = VMDIR_BIND_PROTOCOL_UNSET,
    .dwCacheEntryExpiry = 0
};

VMCACHE_PROVIDER_FUNCTION_TABLE gVmDirCacheTable;
PVMCACHE_PROVIDER_FUNCTION_TABLE gpCacheProvider = &gVmDirCacheTable;
