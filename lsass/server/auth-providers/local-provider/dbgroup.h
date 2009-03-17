
DWORD
LsaProviderLocal_DbFindGroupByName(
    HANDLE  hDb,
    PCSTR   pszDomain,
    PCSTR   pszGroupName,
    DWORD   dwGroupInfoLevel,
    PVOID*  ppGroupInfo
    );


DWORD
LsaProviderLocal_DbFindGroupByName_0(
    HANDLE  hDb,
    PCSTR   pszDomain,
    PCSTR   pszGroupName,
    PVOID*  ppGroupInfo
    );


DWORD
LsaProviderLocal_DbFindGroupByName_1(
    HANDLE  hDb,
    PCSTR   pszDomain,
    PCSTR   pszGroupName,
    PVOID*  ppGroupInfo
    );

DWORD
LsaProviderLocal_DbGetGroupsForUser_0(
    HANDLE  hDb,
    uid_t uid,
    PDWORD  pdwGroupsFound,
    PVOID** pppGroupInfoList
    );

DWORD
LsaProviderLocal_DbGetGroupsForUser_1(
    HANDLE  hDb,
    uid_t uid,
    PDWORD  pdwGroupsFound,
    PVOID** pppGroupInfoList
    );



DWORD
LsaProviderLocal_DbFindGroupByName_0_Unsafe(
    HANDLE  hDb,
    PCSTR   pszDomain,
    PCSTR   pszGroupName,
    PVOID*  ppGroupInfo
    );


DWORD
LsaProviderLocal_DbFindGroupByName_1_Unsafe(
    HANDLE  hDb,
    PCSTR   pszDomain,
    PCSTR   pszGroupName,
    PVOID*  ppGroupInfo
    );

DWORD
LsaProviderLocal_DbFindGroupByName_0_Unsafe(
    HANDLE  hDb,
    PCSTR   pszDomain,
    PCSTR   pszGroupName,
    PVOID*  ppGroupInfo
    );

DWORD
LsaProviderLocal_DbFindGroupByName_1_Unsafe(
    HANDLE  hDb,
    PCSTR   pszDomain,
    PCSTR   pszGroupName,
    PVOID*  ppGroupInfo
    );


DWORD
LsaProviderLocal_DbEnumGroups_0(
    HANDLE    hDb,
    DWORD     dwOffset,
    DWORD     dwLimit,
    PDWORD    pdwNumGroupsFound,
    PVOID**   pppGroupInfoList
    );

DWORD
LsaProviderLocal_DbEnumGroups_1(
    HANDLE    hDb,
    DWORD     dwOffset,
    DWORD     dwLimit,
    PDWORD    pdwNumGroupsFound,
    PVOID**   pppGroupInfoList
    );

DWORD
LsaProviderLocal_DbEnumGroups(
    HANDLE  hDb,
    DWORD   dwGroupInfoLevel,
    DWORD   dwStartingRecordId,
    DWORD   nMaxGroups,
    PDWORD  pdwGroupsFound,
    PVOID** pppGroupInfoList
    );
