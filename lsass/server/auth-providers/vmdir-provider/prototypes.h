/*
 * Copyright (C) VMware. All rights reserved.
 */

// config.c

DWORD
VmDirGetBindProtocol(
    VMDIR_BIND_PROTOCOL* pBindProtocol
    );

DWORD
VmDirCreateBindInfo(
    PVMDIR_BIND_INFO* ppBindInfo
    );

DWORD
VmDirCreateBindInfoPassword(
    PSTR *ppszPassword
    );

PVMDIR_BIND_INFO
VmDirAcquireBindInfo(
    PVMDIR_BIND_INFO pBindInfo
    );

VOID
VmDirReleaseBindInfo(
    PVMDIR_BIND_INFO pBindInfo
    );

DWORD
VmDirGetCacheEntryExpiry(
    DWORD *pdwCacheEntryExpiry
    );

// ldap.c

DWORD
VmDirLdapInitialize(
    PCSTR            pszURI,
    PCSTR            pszUPN,
    PCSTR            pszPassword,
    PCSTR            pszCachePath,
    LDAP**           ppLd
    );

DWORD
VmDirLdapQuerySingleObject(
    LDAP*         pLd,
    PCSTR         pszBaseDN,
    int           scope,
    PCSTR         pszFilter,
    char**        attrs,
    LDAPMessage** ppMessage
    );

DWORD
VmDirLdapQueryObjects(
    LDAP*         pLd,
    PCSTR         pszBaseDN,
    int           scope,
    PCSTR         pszFilter,
    char**        attrs,
    int           sizeLimit,
    LDAPMessage** ppMessage
    );

DWORD
VmDirLdapGetValues(
    LDAP*        pLd,
    LDAPMessage* pMessage,
    PVMDIR_ATTR  pValueArray,
    DWORD        dwNumValues
    );

VOID
VmDirLdapFreeMessage(
    LDAPMessage* pMessage
    );

VOID
VmDirLdapClose(
    LDAP* pLd
    );

// lock.c

DWORD
VmDirRWLockAcquire(
    pthread_rwlock_t* pMutex,
    BOOLEAN           bExclusive,
    PBOOLEAN          pbLocked
    );

DWORD
VmDirRWLockRelease(
    pthread_rwlock_t* pMutex,
    PBOOLEAN          pbLocked
    );

// provider-main.c

DWORD
VmDirFindObjects(
    HANDLE                 hProvider,  /* IN              */
    LSA_FIND_FLAGS         findFlags,  /* IN              */
    LSA_OBJECT_TYPE        objectType, /* IN     OPTIONAL */
    LSA_QUERY_TYPE         queryType,  /* IN              */
    DWORD                  dwCount,    /* IN              */
    LSA_QUERY_LIST         queryList,  /* IN              */
    PLSA_SECURITY_OBJECT** pppObjects  /*    OUT          */
    );

DWORD
VmDirOpenEnumObjects(
    HANDLE          hProvider,    /* IN              */
    PHANDLE         phEnum,       /*    OUT          */
    LSA_FIND_FLAGS  findFlags,    /* IN              */
    LSA_OBJECT_TYPE objectType,   /* IN              */
    PCSTR           pszDomainName /* IN     OPTIONAL */
    );

DWORD
VmDirEnumObjects(
    HANDLE                 hEnum,             /* IN              */
    DWORD                  dwMaxObjectsCount, /* IN              */
    PDWORD                 pdwObjectsCount,   /*    OUT          */
    PLSA_SECURITY_OBJECT** pppObjects         /*    OUT          */
    );

DWORD
VmDirOpenEnumMembers(
    HANDLE         hProvider, /* IN              */
    PHANDLE        phEnum,    /*    OUT          */
    LSA_FIND_FLAGS findFlags, /* IN              */
    PCSTR          pszSid     /* IN              */
    );

DWORD
VmDirEnumMembers(
    HANDLE hEnum,               /* IN              */
    DWORD  dwMaxMemberSidCount, /* IN              */
    PDWORD pdwMemberSidCount,   /*    OUT          */
    PSTR** pppszMemberSids      /*    OUT          */
    );

VOID
VmDirCloseEnum(
    HANDLE hEnum                /* IN OUT          */
    );

DWORD
VmDirQueryMemberOf(
    HANDLE         hProvider,        /* IN              */
    LSA_FIND_FLAGS findFlags,        /* IN              */
    DWORD          dwSidCount,       /* IN              */
    PSTR*          ppszSids,         /* IN              */
    PDWORD         pdwGroupSidCount, /*    OUT          */
    PSTR**         pppszGroupSids    /*    OUT          */
    );

DWORD
VmDirGetSmartCardUserObject(
    HANDLE                hProvider,          /* IN              */
    PLSA_SECURITY_OBJECT* ppObject,           /*    OUT          */
    PSTR*                 ppszSmartCardReader /* IN              */
    );

DWORD
VmDirGetMachineAccountInfoA(
    PCSTR                        dnsDomainName, /* IN     OPTIONAL */
    PLSA_MACHINE_ACCOUNT_INFO_A* ppAccountInfo  /* IN              */
    );

DWORD
VmDirGetMachineAccountInfoW(
    PCSTR                        dnsDomainName, /* IN     OPTIONAL */
    PLSA_MACHINE_ACCOUNT_INFO_W* ppAccountInfo  /*    OUT          */
    );

DWORD
VmDirGetMachinePasswordInfoA(
    PCSTR                         dnsDomainName, /* IN     OPTIONAL */
    PLSA_MACHINE_PASSWORD_INFO_A* ppPasswordInfo /*    OUT          */
    );

DWORD
VmDirGetMachinePasswordInfoW(
    PCSTR                         dnsDomainName, /* IN     OPTIONAL */
    PLSA_MACHINE_PASSWORD_INFO_W* ppPasswordInfo /*    OUT          */
    );

DWORD
VmDirShutdownProvider(
    VOID
    );

DWORD
VmDirOpenHandle(
    HANDLE  hServer,     /* IN              */
    PCSTR   pszInstance, /* IN              */
    PHANDLE phProvider   /*    OUT          */
    );

VOID
VmDirCloseHandle(
    HANDLE hProvider /* IN OUT          */
    );

DWORD
VmDirServicesDomain(
    PCSTR    pszDomain,       /* IN              */
    BOOLEAN* pbServicesDomain /* IN OUT          */
    );

DWORD
VmDirAuthenticateUserPam(
    HANDLE                    hProvider,    /* IN              */
    PLSA_AUTH_USER_PAM_PARAMS pParams,      /* IN              */
    PLSA_AUTH_USER_PAM_INFO*  ppPamAuthInfo /*    OUT          */
    );

DWORD
VmDirAuthenticateUserEx(
    HANDLE                hProvider,   /* IN              */
    PLSA_AUTH_USER_PARAMS pUserParams, /* IN              */
    PLSA_AUTH_USER_INFO*  ppUserInfo   /*    OUT          */
    );

DWORD
VmDirValidateUser(
    HANDLE hProvider,  /* IN              */
    PCSTR  pszLoginId, /* IN              */
    PCSTR  pszPassword /* IN              */
    );

DWORD
VmDirCheckUserInList(
    HANDLE hProvider,  /* IN              */
    PCSTR  pszLoginId, /* IN              */
    PCSTR  pszListName /* IN              */
    );

DWORD
VmDirChangePassword(
    HANDLE hProvider,     /* IN              */
    PCSTR  pszLoginId,    /* IN              */
    PCSTR  pszPassword,   /* IN              */
    PCSTR  pszOldPassword /* IN              */
    );

DWORD
VmDirSetPassword (
    HANDLE hProvider,  /* IN              */
    PCSTR  pszLoginId, /* IN              */
    PCSTR  pszPassword /* IN              */
    );

DWORD
VmDirAddUser (
    HANDLE             hProvider, /* IN              */
    PLSA_USER_ADD_INFO pUserInfo  /* IN              */
    );

DWORD
VmDirModifyUser (
    HANDLE               hProvider,   /* IN              */
    PLSA_USER_MOD_INFO_2 pUserModInfo /* IN              */
    );

DWORD
VmDirAddGroup (
    HANDLE              hProvider, /* IN              */
    PLSA_GROUP_ADD_INFO pGroupInfo /* IN              */
    );

DWORD
VmDirModifyGroup (
    HANDLE                hProvider,    /* IN              */
    PLSA_GROUP_MOD_INFO_2 pGroupModInfo /* IN              */
    );

DWORD
VmDirDeleteObject (
    HANDLE hProvider, /* IN              */
    PCSTR  pszSid     /* IN              */
    );

DWORD
VmDirOpenSession(
    HANDLE hProvider, /* IN              */
    PCSTR  pszLoginId /* IN              */
    );

DWORD
VmDirCloseSession (
    HANDLE hProvider, /* IN              */
    PCSTR  pszLoginId /* IN              */
    );

DWORD
VmDirFindNSSArtefactByKey(
    HANDLE                  hProvider,        /* IN              */
    PCSTR                   pszKeyName,       /* IN              */
    PCSTR                   pszMapName,       /* IN              */
    DWORD                   dwInfoLevel,      /* IN              */
    LSA_NIS_MAP_QUERY_FLAGS dwFlags,          /* IN              */
    PVOID*                  ppNSSArtefactInfo /*    OUT          */
    );

DWORD
VmDirBeginEnumNSSArtefacts(
    HANDLE                  hProvider,   /* IN              */
    DWORD                   dwInfoLevel, /* IN              */
    PCSTR                   pszMapName,  /* IN              */
    LSA_NIS_MAP_QUERY_FLAGS dwFlags,     /* IN              */
    PHANDLE                 phResume     /*    OUT          */
    );

DWORD
VmDirEnumNSSArtefacts (
    HANDLE  hProvider,             /* IN              */
    HANDLE  hResume,               /* IN              */
    DWORD   dwMaxNumGroups,        /* IN              */
    PDWORD  pdwNSSArtefactsFound,  /*    OUT          */
    PVOID** pppNSSArtefactInfoList /*    OUT          */
    );

VOID
VmDirEndEnumNSSArtefacts(
    HANDLE hProvider, /* IN              */
    HANDLE hResume    /* IN              */
    );

DWORD
VmDirGetStatus(
    HANDLE                     hProvider,           /* IN              */
    PLSA_AUTH_PROVIDER_STATUS* ppAuthProviderStatus /*    OUT          */
    );

VOID
VmDirFreeStatus(
    PLSA_AUTH_PROVIDER_STATUS pAuthProviderStatus /* IN OUT          */
    );

DWORD
VmDirRefreshConfiguration(
    VOID
    );

DWORD
VmDirProviderIoControl (
    HANDLE hProvider,           /* IN              */
    uid_t  peerUid,             /* IN              */
    gid_t  peerGID,             /* IN              */
    DWORD  dwIoControlCode,     /* IN              */
    DWORD  dwInputBufferSize,   /* IN              */
    PVOID  pInputBuffer,        /* IN              */
    DWORD* pdwOutputBufferSize, /*    OUT          */
    PVOID* ppOutputBuffer       /*    OUT          */
    );

// refresh.c

DWORD
VmDirStartMachineAccountRefresh(
    PVMDIR_REFRESH_CONTEXT *ppRefreshContext  /*     OUT          */
    );

VOID
VmDirSignalMachineAccountRefresh(
    PVMDIR_REFRESH_CONTEXT pRefreshContext     /* IN              */
    );

VOID
VmDirStopMachineAccountRefresh(
    PVMDIR_REFRESH_CONTEXT pRefreshContext     /* IN              */
    );

// repository.c

BOOLEAN
VmDirMatchesDomain(
    PCSTR pszDomain
    );

DWORD
VmDirFindUserByName(
    PVMDIR_DIR_CONTEXT    pDirContext,
    PCSTR                 pszLoginId,
    PLSA_SECURITY_OBJECT* ppObject
    );

DWORD
VmDirFindUserById(
    PVMDIR_DIR_CONTEXT    pDirContext,
    uid_t                 uid,
    PLSA_SECURITY_OBJECT* ppObject
    );

DWORD
VmDirFindUserBySID(
    PVMDIR_DIR_CONTEXT    pDirContext,
    PCSTR                 pszSID,
    PLSA_SECURITY_OBJECT* ppObject
    );

DWORD
VmDirFindGroupByName(
    PVMDIR_DIR_CONTEXT    pDirContext,
    PCSTR                 pszGroupName,
    PLSA_SECURITY_OBJECT* ppObject
    );

DWORD
VmDirFindGroupById(
    PVMDIR_DIR_CONTEXT    pDirContext,
    gid_t                 gid,
    PLSA_SECURITY_OBJECT* ppObject
    );

DWORD
VmDirFindGroupBySID(
    PVMDIR_DIR_CONTEXT    pDirContext,
    PCSTR                 pszSID,
    PLSA_SECURITY_OBJECT* ppObject
    );

DWORD
VmDirFindDomainSID(
    PVMDIR_DIR_CONTEXT pDirContext,
    PSTR*              ppszDomainSID
    );

DWORD
VmDirFindObjectBySID(
    PVMDIR_DIR_CONTEXT    pDirContext,
    PCSTR                 pszSID,
    PLSA_SECURITY_OBJECT* ppObject
    );

DWORD
VmDirCreateUserEnumHandle(
    PVMDIR_DIR_CONTEXT  pDirContext,
    PVMDIR_ENUM_HANDLE* ppEnumHandle
    );

DWORD
VmDirCreateGroupEnumHandle(
    PVMDIR_DIR_CONTEXT  pDirContext,
    PVMDIR_ENUM_HANDLE* ppEnumHandle
    );

DWORD
VmDirRepositoryEnumObjects(
    PVMDIR_ENUM_HANDLE     pEnumHandle,
    DWORD                  dwMaxCount,
    PLSA_SECURITY_OBJECT** pppObjects,
    PDWORD                 pdwCount
    );

DWORD
VmDirInitEnumMembersHandle(
    PVMDIR_DIR_CONTEXT  pDirContext,
    PCSTR               pszSid,
    PVMDIR_ENUM_HANDLE* ppEnumHandle
    );

DWORD
VmDirRepositoryEnumMembers(
    PVMDIR_ENUM_HANDLE pEnumHandle,
    DWORD              dwMaxCount,
    PSTR**             pppszMemberSids,
    PDWORD             pdwCount
    );

DWORD
VmDirFindMemberships(
    PVMDIR_DIR_CONTEXT pDirContext,
    PCSTR              pszSid,
    PLW_HASH_TABLE     pGroupSidTable
    );

DWORD
VmDirFindSidForDN(
    PVMDIR_DIR_CONTEXT pDirContext,
    PCSTR              pszDN,
    PSTR*              ppszSid
    );

DWORD
VmDirRepositoryVerifyPassword(
    PVMDIR_DIR_CONTEXT pDirContext,
    PCSTR              pszUPN,
    PCSTR              pszPassword
    );

DWORD
VmDirRepositoryChangePassword(
    PVMDIR_DIR_CONTEXT pDirContext,
    PCSTR              pszUPN,
    PCSTR              pszNewPassword,
    PCSTR              pszOldPassword
    );

// utils.c

DWORD
VmDirCrackLoginId(
    PCSTR pszLoginId,
    PSTR* ppszDomain,
    PSTR* ppszAccount
    );

DWORD
VmDirGetBindInfo(
    PVMDIR_BIND_INFO* ppBindInfo
    );

DWORD
VmDirGetDomainFromDN(
    PCSTR pszDN,
    PSTR* ppszDomain
    );

DWORD
VmDirGetDefaultSearchBase(
    PCSTR pszBindDN,
    PSTR* ppszSearchBase
    );

DWORD
VmDirGetRID(
    PCSTR  pszObjectSid,
    PDWORD pdwRID
    );

DWORD
VmDirGetRIDFromUID(
    DWORD uid,
    PSTR *pszRid
    );

DWORD
VmDirInitializeUserLoginCredentials(
    IN PCSTR pszUPN,
    IN PCSTR pszPassword,
    IN uid_t uid,
    IN gid_t gid,
    OUT PDWORD pdwGoodUntilTime
    );

VOID
InitializeMemCacheProvider(
    OUT PVMCACHE_PROVIDER_FUNCTION_TABLE pCacheTable
    );
