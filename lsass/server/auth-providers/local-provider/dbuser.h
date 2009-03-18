DWORD
LsaProviderLocal_DbFindUserByName(
    HANDLE  hDb,
    PCSTR   pszDomain,
    PCSTR   pszUserName,
    DWORD   dwUserInfoLevel,
    PVOID*  ppUserInfo
    );;


DWORD
LsaProviderLocal_DbEnumUsers_0(
    HANDLE hDb,
    DWORD  dwOffset,
    DWORD  dwLimit,
    PDWORD pdwNumUsersFound,
    PVOID** pppUserInfoList
    );;


DWORD
LsaProviderLocal_DbFindUserByName_0(
    HANDLE hDb,
    PCSTR  pszUserName,
    PVOID* ppUserInfo
    );;


DWORD
LsaProviderLocal_DbFindUserByName_1(
    HANDLE hDb,
    PCSTR  pszUserName,
    PVOID* ppUserInfo
    );


DWORD
LsaProviderLocal_DbFindUserByName_2(
    HANDLE hDb,
    PCSTR  pszUserName,
    PVOID* ppUserInfo
    );



DWORD
LsaProviderLocal_DbEnumUsers_1(
    HANDLE hDb,
    DWORD  dwOffset,
    DWORD  dwLimit,
    PDWORD pdwNumUsersFound,
    PVOID** pppUserInfoList
    );


DWORD
LsaProviderLocal_DbEnumUsers_2(
    HANDLE  hDb,
    DWORD   dwOffset,
    DWORD   dwLimit,
    PDWORD  pdwNumUsersFound,
    PVOID** pppUserInfoList
    );

DWORD
LsaProviderLocal_DbEnumUsers(
    HANDLE  hDb,
    DWORD   dwUserInfoLevel,
    DWORD   dwStartingRecordId,
    DWORD   nMaxUsers,
    PDWORD  pdwNumUsersFound,
    PVOID** pppUserInfoList
    );


DWORD
LsaProviderLocal_DbFindUserById(
    HANDLE hDb,
    uid_t  uid,
    DWORD  dwUserInfoLevel,
    PVOID* ppUserInfo
    );


DWORD
LsaProviderLocal_DbGetGroupsForUser_0_Unsafe(
    HANDLE  hDb,
    uid_t uid,
    PDWORD  pdwGroupsFound,
    PVOID** pppGroupInfoList
    );


DWORD
LsaProviderLocal_DbGetGroupsForUser_1_Unsafe(
    HANDLE  hDb,
    uid_t uid,
    PDWORD  pdwGroupsFound,
    PVOID** pppGroupInfoList
    );

DWORD
LsaProviderLocal_DbGetGroupsForUser_0_Unsafe(
    HANDLE  hDb,
    uid_t uid,
    PDWORD  pdwGroupsFound,
    PVOID** pppGroupInfoList
    );
