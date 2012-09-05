/*
 * Copyright (C) VMware. All rights reserved.
 */

// lock.c

DWORD
SampleRWLockAcquire(
	pthread_rwlock_t* pMutex,
	BOOLEAN           bExclusive,
	PBOOLEAN          pbLocked
	);

DWORD
SampleRWLockRelease(
	pthread_rwlock_t* pMutex,
	PBOOLEAN          pbLocked
	);

// provider-main.c

DWORD
SampleFindObjects(
    HANDLE                 hProvider,  /* IN              */
    LSA_FIND_FLAGS         findFlags,  /* IN              */
    LSA_OBJECT_TYPE        objectType, /* IN     OPTIONAL */
    LSA_QUERY_TYPE         queryType,  /* IN              */
    DWORD                  dwCount,    /* IN              */
    LSA_QUERY_LIST         queryList,  /* IN              */
    PLSA_SECURITY_OBJECT** pppObjects  /*    OUT          */
    );

DWORD
SampleOpenEnumObjects(
    HANDLE          hProvider,    /* IN              */
    PHANDLE         phEnum,       /*    OUT          */
    LSA_FIND_FLAGS  findFlags,    /* IN              */
    LSA_OBJECT_TYPE objectType,   /* IN              */
    PCSTR           pszDomainName /* IN     OPTIONAL */
    );

DWORD
SampleEnumObjects(
    HANDLE                 hEnum,             /* IN              */
    DWORD                  dwMaxObjectsCount, /* IN              */
    PDWORD                 pdwObjectsCount,   /*    OUT          */
    PLSA_SECURITY_OBJECT** pppObjects         /*    OUT          */
    );

DWORD
SampleOpenEnumMembers(
    HANDLE         hProvider, /* IN              */
    PHANDLE        phEnum,    /*    OUT          */
    LSA_FIND_FLAGS findFlags, /* IN              */
    PCSTR          pszSid     /* IN              */
    );

DWORD
SampleEnumMembers(
    HANDLE hEnum,               /* IN              */
    DWORD  dwMaxMemberSidCount, /* IN              */
    PDWORD pdwMemberSidCount,   /*    OUT          */
    PSTR** pppszMemberSids      /*    OUT          */
    );

VOID
SampleCloseEnum(
    HANDLE hEnum                /* IN OUT          */
    );

DWORD
SampleQueryMemberOf(
    HANDLE         hProvider,        /* IN              */
    LSA_FIND_FLAGS findFlags,        /* IN              */
    DWORD          dwSidCount,       /* IN              */
    PSTR*          ppszSids,         /* IN              */
    PDWORD         pdwGroupSidCount, /*    OUT          */
    PSTR**         pppszGroupSids    /*    OUT          */
    );

DWORD
SampleGetSmartCardUserObject(
    HANDLE                hProvider,          /* IN              */
    PLSA_SECURITY_OBJECT* ppObject,           /*    OUT          */
    PSTR*                 ppszSmartCardReader /* IN              */
    );

DWORD
SampleGetMachineAccountInfoA(
    PCSTR                        dnsDomainName, /* IN     OPTIONAL */
    PLSA_MACHINE_ACCOUNT_INFO_A* ppAccountInfo  /* IN              */
    );

DWORD
SampleGetMachineAccountInfoW(
    PCSTR                        dnsDomainName, /* IN     OPTIONAL */
    PLSA_MACHINE_ACCOUNT_INFO_W* ppAccountInfo  /*    OUT          */
    );

DWORD
SampleGetMachinePasswordInfoA(
    PCSTR                         dnsDomainName, /* IN     OPTIONAL */
    PLSA_MACHINE_PASSWORD_INFO_A* ppPasswordInfo /*    OUT          */
    );

DWORD
SampleGetMachinePasswordInfoW(
    PCSTR                         dnsDomainName, /* IN     OPTIONAL */
    PLSA_MACHINE_PASSWORD_INFO_W* ppPasswordInfo /*    OUT          */
    );

DWORD
SampleShutdownProvider(
    VOID
    );

DWORD
SampleOpenHandle(
    HANDLE  hServer,     /* IN              */
    PCSTR   pszInstance, /* IN              */
    PHANDLE phProvider   /*    OUT          */
    );

VOID
SampleCloseHandle(
    HANDLE hProvider /* IN OUT          */
    );

DWORD
SampleServicesDomain(
    PCSTR    pszDomain,       /* IN              */
    BOOLEAN* pbServicesDomain /* IN OUT          */
    );

DWORD
SampleAuthenticateUserPam(
    HANDLE                    hProvider,    /* IN              */
    PLSA_AUTH_USER_PAM_PARAMS pParams,      /* IN              */
    PLSA_AUTH_USER_PAM_INFO*  ppPamAuthInfo /*    OUT          */
    );

DWORD
SampleAuthenticateUserEx(
    HANDLE                hProvider,   /* IN              */
    PLSA_AUTH_USER_PARAMS pUserParams, /* IN              */
    PLSA_AUTH_USER_INFO*  ppUserInfo   /*    OUT          */
    );

DWORD
SampleValidateUser(
    HANDLE hProvider,  /* IN              */
    PCSTR  pszLoginId, /* IN              */
    PCSTR  pszPassword /* IN              */
    );

DWORD
SampleCheckUserInList(
    HANDLE hProvider,  /* IN              */
    PCSTR  pszLoginId, /* IN              */
    PCSTR  pszListName /* IN              */
    );

DWORD
SampleChangePassword(
    HANDLE hProvider,     /* IN              */
    PCSTR  pszLoginId,    /* IN              */
    PCSTR  pszPassword,   /* IN              */
    PCSTR  pszOldPassword /* IN              */
    );

DWORD
SampleSetPassword (
    HANDLE hProvider,  /* IN              */
    PCSTR  pszLoginId, /* IN              */
    PCSTR  pszPassword /* IN              */
    );

DWORD
SampleAddUser (
    HANDLE             hProvider, /* IN              */
    PLSA_USER_ADD_INFO pUserInfo  /* IN              */
    );

DWORD
SampleModifyUser (
    HANDLE               hProvider,   /* IN              */
    PLSA_USER_MOD_INFO_2 pUserModInfo /* IN              */
    );

DWORD
SampleAddGroup (
    HANDLE              hProvider, /* IN              */
    PLSA_GROUP_ADD_INFO pGroupInfo /* IN              */
    );

DWORD
SampleModifyGroup (
    HANDLE                hProvider,    /* IN              */
    PLSA_GROUP_MOD_INFO_2 pGroupModInfo /* IN              */
    );

DWORD
SampleDeleteObject (
    HANDLE hProvider, /* IN              */
    PCSTR  pszSid     /* IN              */
    );

DWORD
SampleOpenSession(
    HANDLE hProvider, /* IN              */
    PCSTR  pszLoginId /* IN              */
    );

DWORD
SampleCloseSession (
    HANDLE hProvider, /* IN              */
    PCSTR  pszLoginId /* IN              */
    );

DWORD
SampleFindNSSArtefactByKey(
    HANDLE                  hProvider,        /* IN              */
    PCSTR                   pszKeyName,       /* IN              */
    PCSTR                   pszMapName,       /* IN              */
    DWORD                   dwInfoLevel,      /* IN              */
    LSA_NIS_MAP_QUERY_FLAGS dwFlags,          /* IN              */
    PVOID*                  ppNSSArtefactInfo /*    OUT          */
    );

DWORD
SampleBeginEnumNSSArtefacts(
    HANDLE                  hProvider,   /* IN              */
    DWORD                   dwInfoLevel, /* IN              */
    PCSTR                   pszMapName,  /* IN              */
    LSA_NIS_MAP_QUERY_FLAGS dwFlags,     /* IN              */
    PHANDLE                 phResume     /*    OUT          */
    );

DWORD
SampleEnumNSSArtefacts (
    HANDLE  hProvider,             /* IN              */
    HANDLE  hResume,               /* IN              */
    DWORD   dwMaxNumGroups,        /* IN              */
    PDWORD  pdwNSSArtefactsFound,  /*    OUT          */
    PVOID** pppNSSArtefactInfoList /*    OUT          */
    );

VOID
SampleEndEnumNSSArtefacts(
    HANDLE hProvider, /* IN              */
    HANDLE hResume    /* IN              */
    );

DWORD
SampleGetStatus(
    HANDLE                     hProvider,           /* IN              */
    PLSA_AUTH_PROVIDER_STATUS* ppAuthProviderStatus /*    OUT          */
    );

VOID
SampleFreeStatus(
    PLSA_AUTH_PROVIDER_STATUS pAuthProviderStatus /* IN OUT          */
    );

DWORD
SampleRefreshConfiguration(
    VOID
    );

DWORD
SampleProviderIoControl (
    HANDLE hProvider,           /* IN              */
    uid_t  peerUid,             /* IN              */
    gid_t  peerGID,             /* IN              */
    DWORD  dwIoControlCode,     /* IN              */
    DWORD  dwInputBufferSize,   /* IN              */
    PVOID  pInputBuffer,        /* IN              */
    DWORD* pdwOutputBufferSize, /*    OUT          */
    PVOID* ppOutputBuffer       /*    OUT          */
    );

// repository.c

DWORD
SampleCrackLoginId(
	PCSTR pszLoginId,
	PSTR* ppszDomain,
	PSTR* ppszAccount
	);

BOOLEAN
SampleMatchesDomain(
	PCSTR pszDomain
	);

DWORD
SampleGetDomain(
	PSTR* ppszDomain
	);

DWORD
SampleFindUserByName(
	PCSTR                 pszLoginId,
	PLSA_SECURITY_OBJECT* ppObject
	);

DWORD
SampleFindUserById(
	uid_t                 uid,
	PLSA_SECURITY_OBJECT* ppObject
	);

DWORD
SampleFindGroupByName(
	PCSTR                 pszGroupName,
	PLSA_SECURITY_OBJECT* ppObject
	);

DWORD
SampleFindGroupById(
	gid_t                 gid,
	PLSA_SECURITY_OBJECT* ppObject
	);

DWORD
SampleFindObjectBySID(
	PCSTR                 pszSID,
	PLSA_SECURITY_OBJECT* ppObject
	);

DWORD
SampleInitEnumHandle(
	PSAMPLE_ENUM_HANDLE pEnumHandle
	);

DWORD
SampleRepositoryEnumObjects(
	PSAMPLE_ENUM_HANDLE     pEnumHandle,
	PLSA_SECURITY_OBJECT** pppObjects,
	DWORD                  dwCount
	);

DWORD
SampleInitEnumMembersHandle(
	PLSA_SECURITY_OBJECT pObject,
	PSAMPLE_ENUM_HANDLE   pEnumHandle
	);

DWORD
SampleRepositoryEnumMembers(
	PSAMPLE_ENUM_HANDLE pEnumHandle,
	PSTR**             pppszMemberSids,
	DWORD              dwCount
	);

DWORD
SampleFindMemberships(
	PCSTR          pszSid,
	PLW_HASH_TABLE pGroupSidTable
	);

DWORD
SampleRepositoryVerifyPassword(
	PCSTR pszUsername,
	PCSTR pszPassword
	);

DWORD
SampleRepositoryChangePassword(
    PCSTR pszAcctName,
	PCSTR pszNewPassword,
	PCSTR pszOldPassword
	);
