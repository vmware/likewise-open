
#include "includes.h"

NTSTATUS
SamDBAddObject(
    HANDLE hBindHandle,
    PWSTR ObjectDN,
    )
{
    NTSTATUS ntStatus = 0;
    PDIRECTORY_CONTEXT pDirectoryContext = hBindHandle;

    ntStatus = SamDbParseDN(ObjectDN,&pszObjectName, &dwType);
    BAIL_ON_NT_STATUS(ntStatus);

    switch (dwType) {

        case SAMDB_USER:
            SamDbDeleteUser(
                    hDirectory,
                    pszObjectName
                    );
            break;

        case SAMDB_GROUP:
            SamDbDeleteGroup(
                    hDirectory,
                    pszObjectName
                    );
            break;
    }

    return ntStatus;
}

NTSTATUS
SamDbAddUser(
    HANDLE hDirectory,
    PWSTR pszObjectName,
    DIRECTORY_MODS Modifications[]
    )
{
    NTSTATUS ntStatus = 0;
    DWORD dwError = 0;
    sqlite3* pDbHandle = (sqlite3*)hDb;
    PSTR pszError = NULL;
    PSTR pszQuery = NULL;
    PLSA_USER_INFO_0 pUser = NULL;
    BOOLEAN bReleaseLock = FALSE;
    PVOID pExistingUserInfo = NULL;
    PVOID pGroupInfo = NULL;
    DWORD  dwExistingUserInfoLevel = 0;
    DWORD  dwGroupInfoLevel = 0;


    ntStatus =SamDbFindUser(
                    hDirectory,
    if (dwUserInfoLevel != 0) {
        dwError = LSA_ERROR_INVALID_USER_INFO_LEVEL;
        BAIL_ON_LSA_ERROR(dwError);
    }

    pUser = (PLSA_USER_INFO_0)pUserInfo;

    ENTER_RW_WRITER_LOCK;
    bReleaseLock = TRUE;

    if (IsNullOrEmptyString(pUser->pszName)) {
        dwError = LSA_ERROR_INVALID_LOGIN_ID;
        BAIL_ON_LSA_ERROR(dwError);
    }

    if (IsNullOrEmptyString(pUser->pszHomedir)) {
        dwError = LSA_ERROR_INVALID_HOMEDIR;
        BAIL_ON_LSA_ERROR(dwError);
    }

    // This will not let root be added - which is ok.
    if (!pUser->uid) {

       // We got a write lock; nobody can steal this uid from us
       dwError = LsaProviderLocal_DbGetNextAvailableUid_Unsafe(
                           hDb,
                           &pUser->uid
                           );
       BAIL_ON_LSA_ERROR(dwError);

    } else {

       dwError = LsaProviderLocal_DbFindUserById_0_Unsafe(
                           hDb,
                           pUser->uid,
                           &pExistingUserInfo
                           );
       if (dwError) {
          if (dwError == LSA_ERROR_NO_SUCH_USER) {
              dwError = 0;
          } else {
             BAIL_ON_LSA_ERROR(dwError);
          }
       } else {
          dwError = LSA_ERROR_USER_EXISTS;
          BAIL_ON_LSA_ERROR(dwError);
       }
    }

    dwError = LsaProviderLocal_DbFindUserByName_0_Unsafe(
                           hDb,
                           pUser->pszName,
                           &pExistingUserInfo);
    if (dwError) {
           if (dwError == LSA_ERROR_NO_SUCH_USER) {
               dwError = 0;
           } else {
               BAIL_ON_LSA_ERROR(dwError);
           }
    } else {
           dwError = LSA_ERROR_USER_EXISTS;
           BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = LsaProviderLocal_DbFindGroupById_0_Unsafe(
                        hDb,
                        pUser->gid,
                        &pGroupInfo
                        );
    BAIL_ON_LSA_ERROR(dwError);

    pszQuery = sqlite3_mprintf(DB_QUERY_INSERT_USER,
                               pUser->pszName,
                               pUser->pszPasswd,
                               pUser->uid,
                               pUser->gid,
                               pUser->pszGecos,
                               pUser->pszHomedir,
                               pUser->pszShell,
                               pUser->pszName);

    dwError = sqlite3_exec(pDbHandle,
                           pszQuery,
                           NULL,
                           NULL,
                           &pszError);
    BAIL_ON_LSA_ERROR(dwError);

cleanup:

    if (pszQuery) {
       sqlite3_free(pszQuery);
    }

    if (bReleaseLock) {
        LEAVE_RW_WRITER_LOCK;
    }

    if (pExistingUserInfo) {
        LsaFreeUserInfo(dwExistingUserInfoLevel, pExistingUserInfo);
    }

    if (pGroupInfo) {
        LsaFreeGroupInfo(dwGroupInfoLevel, pGroupInfo);
    }

    return dwError;

error:

    if (!IsNullOrEmptyString(pszError)) {
       LSA_LOG_ERROR("%s", pszError);
    }

    goto cleanup;
}


NTSTATUS
SamDbAddGroup(
    HANDLE hDirectory,
    PWSTR pszObjectName
    )
{
    NTSTATUS ntStatus = 0;

    return ntStatus;
}
