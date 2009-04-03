DWORD
LsaLPDbFindGroupByName(
    HANDLE  hDb,
    PCSTR   pszDomain,
    PCSTR   pszGroupName,
    DWORD   dwGroupInfoLevel,
    PVOID*  ppGroupInfo
    );

DWORD
LsaLPDbFindGroupByName_0(
    HANDLE  hDb,
    PCSTR   pszDomain,
    PCSTR   pszGroupName,
    PVOID*  ppGroupInfo
    );

DWORD
LsaLPDbFindGroupByName_1(
    HANDLE  hDb,
    PCSTR   pszDomain,
    PCSTR   pszGroupName,
    PVOID*  ppGroupInfo
    );

DWORD
LsaLPDbGetGroupsForUser_0(
    HANDLE  hDb,
    uid_t uid,
    PDWORD  pdwGroupsFound,
    PVOID** pppGroupInfoList
    );


DWORD
LsaLPDbGetGroupsForUser_1(
    HANDLE  hDb,
    uid_t uid,
    PDWORD  pdwGroupsFound,
    PVOID** pppGroupInfoList
    );

DWORD
LsaLPDbFindGroupByName_0_Unsafe(
    HANDLE  hDb,
    PCSTR   pszDomain,
    PCSTR   pszGroupName,
    PVOID*  ppGroupInfo
    );

DWORD
LsaLPDbFindGroupByName_1_Unsafe(
    HANDLE  hDb,
    PCSTR   pszDomain,
    PCSTR   pszGroupName,
    PVOID*  ppGroupInfo
    );


DWORD
LsaLPDbEnumGroups_0(
    HANDLE    hDb,
    DWORD     dwOffset,
    DWORD     dwLimit,
    PDWORD    pdwNumGroupsFound,
    PVOID**   pppGroupInfoList
    );

DWORD
LsaLPDbEnumGroups_1(
    HANDLE    hDb,
    DWORD     dwOffset,
    DWORD     dwLimit,
    PDWORD    pdwNumGroupsFound,
    PVOID**   pppGroupInfoList
    );

DWORD
LsaLPDbEnumGroups(
    HANDLE  hDb,
    DWORD   dwGroupInfoLevel,
    DWORD   dwStartingRecordId,
    DWORD   nMaxGroups,
    PDWORD  pdwGroupsFound,
    PVOID** pppGroupInfoList
    );

DWORD
LsaLPDbFindGroupById(
    HANDLE  hDb,
    gid_t   gid,
    DWORD   dwGroupInfoLevel,
    PVOID*  ppGroupInfo
    );


DWORD
LsaLPDbFindGroupById_0(
    HANDLE hDb,
    gid_t  gid,
    PVOID* ppGroupInfo
    );


DWORD
LsaLPDbFindGroupById_1(
    HANDLE hDb,
    gid_t  gid,
    PVOID* ppGroupInfo
    );


DWORD
LsaLPDbFindGroupById_0_Unsafe(
    HANDLE hDb,
    gid_t  gid,
    PVOID* ppGroupInfo
    );;


DWORD
LsaLPDbFindGroupById_1_Unsafe(
    HANDLE hDb,
    gid_t  gid,
    PVOID* ppGroupInfo
    );

