/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

/*
 * Copyright Likewise Software
 * All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or (at
 * your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * for more details.  You should have received a copy of the GNU General
 * Public License along with this program.  If not, see
 * <http://www.gnu.org/licenses/>.
 *
 * LIKEWISE SOFTWARE MAKES THIS SOFTWARE AVAILABLE UNDER OTHER LICENSING
 * TERMS AS WELL.  IF YOU HAVE ENTERED INTO A SEPARATE LICENSE AGREEMENT
 * WITH LIKEWISE SOFTWARE, THEN YOU MAY ELECT TO USE THE SOFTWARE UNDER THE
 * TERMS OF THAT SOFTWARE LICENSE AGREEMENT INSTEAD OF THE TERMS OF THE GNU
 * GENERAL PUBLIC LICENSE, NOTWITHSTANDING THE ABOVE NOTICE.  IF YOU
 * HAVE QUESTIONS, OR WISH TO REQUEST A COPY OF THE ALTERNATE LICENSING
 * TERMS OFFERED BY LIKEWISE SOFTWARE, PLEASE CONTACT LIKEWISE SOFTWARE AT
 * license@likewisesoftware.com
 */

/*
 * Copyright (C) Likewise Software. All rights reserved.
 *
 * Module Name:
 *
 *        main.c
 *
 * Abstract:
 *
 *        Likewise Security and Authentication Subsystem (LSASS)
 *        Utility to manipulate the AD cache
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 */

#include "config.h"
#include "lsasystem.h"
#include "lsadef.h"
#include "lsa/lsa.h"
#include "lsa/ad.h"

#include "lsaclient.h"
#include "lsaadprovider.h"
#include "lsaipc.h"

#define ACTION_NONE          0
#define ACTION_DELETE_ALL    1
#define ACTION_DELETE_USER   2
#define ACTION_DELETE_GROUP  3
#define ACTION_ENUM_USERS    4
#define ACTION_ENUM_GROUPS   5

#define LW_PRINTF_STRING(x) ((x) ? (x) : "<null>")

static
PSTR
GetProgramName(
    PSTR pszFullProgramPath
    );

static
DWORD
ParseArgs(
    int    argc,
    char*  argv[],
    DWORD* pdwAction,
    PSTR*  ppszName,
    uid_t* pUID,
    gid_t* pGID,
    DWORD* pdwLevel,
    DWORD* pdwBatchSize
    );

static
VOID
ShowUsage(
    PCSTR pszProgramName
    );

static
DWORD
EnumerateUsers(
    HANDLE hLsaConnection,
    DWORD  dwUserInfoLevel,
    DWORD  dwBatchSize
    );

static
DWORD
EnumerateGroups(
    HANDLE hLsaConnection,
    DWORD  dwGroupInfoLevel,
    DWORD  dwBatchSize
    );

static
VOID
PrintUserInfo_0(
    PLSA_USER_INFO_0 pUserInfo
    );

static
VOID
PrintUserInfo_1(
    PLSA_USER_INFO_1 pUserInfo
    );

static
VOID
PrintUserInfo_2(
    PLSA_USER_INFO_2 pUserInfo
    );

static
VOID
PrintGroupInfo_0(
    PLSA_GROUP_INFO_0 pGroupInfo
    );

static
VOID
PrintGroupInfo_1(
    PLSA_GROUP_INFO_1 pGroupInfo
    );

static
DWORD
MapErrorCode(
    DWORD dwError
    );

static
BOOLEAN
IsUnsignedInteger(
    PCSTR pszIntegerCandidate
    );

int
ad_cache_main(
    int argc,
    char* argv[]
    )
{
    DWORD dwError = 0;
    HANDLE hLsaConnection = (HANDLE)NULL;
    size_t dwErrorBufferSize = 0;
    BOOLEAN bPrintOrigError = TRUE;
    PSTR    pszOperation = "complete operation";
    DWORD   dwAction = ACTION_NONE;
    PSTR    pszName = NULL;
    uid_t   uid = 0;
    gid_t   gid = 0;
    DWORD   dwInfoLevel = 0;
    DWORD   dwBatchSize = 10;

    if (argc < 2 ||
        (strcmp(argv[1], "--help") == 0) ||
        (strcmp(argv[1], "-h") == 0))
    {
        ShowUsage(GetProgramName(argv[0]));
        exit(0);
    }

    dwError = ParseArgs(
                  argc,
                  argv,
                  &dwAction,
                  &pszName,
                  &uid,
                  &gid,
                  &dwInfoLevel,
                  &dwBatchSize);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaOpenServer(&hLsaConnection);
    BAIL_ON_LSA_ERROR(dwError);

    switch (dwAction)
    {
        case ACTION_DELETE_ALL:
            pszOperation = "empty cache";

            dwError = LsaAdEmptyCache(hLsaConnection);
            BAIL_ON_LSA_ERROR(dwError);

            fprintf(stdout, "The cache has been emptied successfully.\n");

            break;

        case ACTION_DELETE_USER:
            pszOperation = "delete user";

            if ( pszName )
            {
                dwError = LsaAdRemoveUserByNameFromCache(
                              hLsaConnection,
                              pszName);
                BAIL_ON_LSA_ERROR(dwError);
            }
            else
            {
                dwError = LsaAdRemoveUserByIdFromCache(
                              hLsaConnection,
                              uid);
                BAIL_ON_LSA_ERROR(dwError);
            }

            fprintf(stdout, "The user has been deleted from the cache successfully.\n");

            break;

        case ACTION_DELETE_GROUP:
            pszOperation = "delete group";

            if ( pszName )
            {
                dwError = LsaAdRemoveGroupByNameFromCache(
                              hLsaConnection,
                              pszName);
                BAIL_ON_LSA_ERROR(dwError);
            }
            else
            {
                dwError = LsaAdRemoveGroupByIdFromCache(
                              hLsaConnection,
                              gid);
                BAIL_ON_LSA_ERROR(dwError);
            }

            fprintf(stdout, "The group has been deleted from the cache successfully.\n");

            break;

        case ACTION_ENUM_USERS:
            pszOperation = "enumerate users";

            dwError = EnumerateUsers(
                          hLsaConnection,
                          dwInfoLevel,
                          dwBatchSize);
            BAIL_ON_LSA_ERROR(dwError);

            break;

        case ACTION_ENUM_GROUPS:
            pszOperation = "enumerate groups";

            dwError = EnumerateGroups(
                          hLsaConnection,
                          dwInfoLevel,
                          dwBatchSize);
            BAIL_ON_LSA_ERROR(dwError);

        break;
    }

cleanup:

    if (hLsaConnection != (HANDLE)NULL) {
        LsaCloseServer(hLsaConnection);
    }

    return dwError;

error:

    dwError = MapErrorCode(dwError);

    dwErrorBufferSize = LwGetErrorString(dwError, NULL, 0);

    if (dwErrorBufferSize > 0)
    {
        DWORD dwError2 = 0;
        PSTR   pszErrorBuffer = NULL;

        dwError2 = LwAllocateMemory(
                    dwErrorBufferSize,
                    (PVOID*)&pszErrorBuffer);

        if (!dwError2)
        {
            DWORD dwLen = LwGetErrorString(dwError, pszErrorBuffer, dwErrorBufferSize);

            if ((dwLen == dwErrorBufferSize) && !LW_IS_NULL_OR_EMPTY_STR(pszErrorBuffer))
            {
                fprintf(stderr,
                        "Failed to %s.  Error code %u (%s).\n%s\n",
                        pszOperation,
                        dwError,
                        LW_PRINTF_STRING(LwWin32ErrorToName(dwError)),
                        pszErrorBuffer);
                bPrintOrigError = FALSE;
            }
        }

        LW_SAFE_FREE_STRING(pszErrorBuffer);
    }

    if (bPrintOrigError)
    {
        fprintf(stderr,
                "Failed to %s.  Error code %u (%s).\n",
                pszOperation,
                dwError,
                LW_PRINTF_STRING(LwWin32ErrorToName(dwError)));
    }

    goto cleanup;
}

static
PSTR
GetProgramName(
    PSTR pszFullProgramPath
    )
{
    if (pszFullProgramPath == NULL || *pszFullProgramPath == '\0') {
        return NULL;
    }

    // start from end of the string
    PSTR pszNameStart = pszFullProgramPath + strlen(pszFullProgramPath);
    do {
        if (*(pszNameStart - 1) == '/') {
            break;
        }

        pszNameStart--;

    } while (pszNameStart != pszFullProgramPath);

    return pszNameStart;
}

static
DWORD
ParseArgs(
    int    argc,
    char*  argv[],
    DWORD* pdwAction,
    PSTR*  ppszName,
    uid_t* pUID,
    gid_t* pGID,
    DWORD* pdwInfoLevel,
    DWORD* pdwBatchSize
    )
{
    typedef enum {
            PARSE_MODE_OPEN = 0,
            PARSE_MODE_NAME,
            PARSE_MODE_UID,
            PARSE_MODE_GID,
            PARSE_MODE_LEVEL,
            PARSE_MODE_BATCHSIZE,
            PARSE_MODE_DONE
    } ParseMode;

    DWORD dwError = 0;
    int iArg = 1;
    PSTR pszArg = NULL;
    ParseMode parseMode = PARSE_MODE_OPEN;
    DWORD dwAction = ACTION_NONE;
    PSTR  pszName = NULL;
    uid_t uid = 0;
    gid_t gid = 0;
    DWORD dwInfoLevel = 0;
    DWORD dwBatchSize = 10;

    do {
        pszArg = argv[iArg++];
        if (pszArg == NULL || *pszArg == '\0')
        {
            break;
        }

        switch (parseMode)
        {
            case PARSE_MODE_OPEN:

                if ((strcmp(pszArg, "--help") == 0) ||
                    (strcmp(pszArg, "-h") == 0))
                {
                    ShowUsage(GetProgramName(argv[0]));
                    exit(0);
                }
                else if (!strcmp(pszArg, "--delete-all")) {
                    dwAction = ACTION_DELETE_ALL;
                    parseMode = PARSE_MODE_DONE;
                }
                else if (!strcmp(pszArg, "--delete-user")) {
                    dwAction = ACTION_DELETE_USER;
                }
                else if (!strcmp(pszArg, "--delete-group")) {
                    dwAction = ACTION_DELETE_GROUP;
                }
                else if (!strcmp(pszArg, "--enum-users")) {
                    dwAction = ACTION_ENUM_USERS;
                }
                else if (!strcmp(pszArg, "--enum-groups")) {
                    dwAction = ACTION_ENUM_GROUPS;
                }
                else if (!strcmp(pszArg, "--name")) {
                    parseMode = PARSE_MODE_NAME;
                }
                else if (!strcmp(pszArg, "--uid")) {
                    parseMode = PARSE_MODE_UID;
                }
                else if (!strcmp(pszArg, "--gid")) {
                    parseMode = PARSE_MODE_GID;
                }
                else if (!strcmp(pszArg, "--level")) {
                    parseMode = PARSE_MODE_LEVEL;
                }
                else if (!strcmp(pszArg, "--batchsize")) {
                    parseMode = PARSE_MODE_BATCHSIZE;
                }
                else
                {
                    ShowUsage(GetProgramName(argv[0]));
                    exit(1);
                }
                break;

            case PARSE_MODE_NAME:
                dwError = LwAllocateString(pszArg, &pszName);
                BAIL_ON_LSA_ERROR(dwError);
                parseMode = PARSE_MODE_OPEN;

                break;

            case PARSE_MODE_UID:
                if (!IsUnsignedInteger(pszArg))
                {
                    fprintf(stderr, "Please enter a UID which is an unsigned integer.\n");
                    ShowUsage(GetProgramName(argv[0]));
                    exit(1);
                }
                uid = atoi(pszArg);
                parseMode = PARSE_MODE_OPEN;

                break;

            case PARSE_MODE_GID:
                if (!IsUnsignedInteger(pszArg))
                {
                    fprintf(stderr, "Please enter a GID which is an unsigned integer.\n");
                    ShowUsage(GetProgramName(argv[0]));
                    exit(1);
                }
                gid = atoi(pszArg);
                parseMode = PARSE_MODE_OPEN;

                break;

            case PARSE_MODE_LEVEL:
                if (!IsUnsignedInteger(pszArg))
                {
                    fprintf(stderr, "Please enter a valid level.\n");
                    ShowUsage(GetProgramName(argv[0]));
                    exit(1);
                }
                dwInfoLevel = atoi(pszArg);
                parseMode = PARSE_MODE_OPEN;

                break;

            case PARSE_MODE_BATCHSIZE:
                if (!IsUnsignedInteger(pszArg))
                {
                    fprintf(stderr, "Please enter a valid batch size.\n");
                    ShowUsage(GetProgramName(argv[0]));
                    exit(1);
                }
                dwBatchSize = atoi(pszArg);
                if ((dwBatchSize < 0) ||
                    (dwBatchSize > 1000)) {
                    fprintf(stderr, "Please enter a valid batch size.\n");
                    ShowUsage(GetProgramName(argv[0]));
                    exit(1);
                }
                parseMode = PARSE_MODE_OPEN;

                break;

            case PARSE_MODE_DONE:
                ShowUsage(GetProgramName(argv[0]));
                exit(1);
        }

    } while (iArg < argc);

    if (parseMode != PARSE_MODE_OPEN && parseMode != PARSE_MODE_DONE)
    {
        ShowUsage(GetProgramName(argv[0]));
        exit(1);
    }

    if ( dwAction == ACTION_NONE )
    {
        fprintf(stderr, "Please specify a valid action.\n");
        ShowUsage(GetProgramName(argv[0]));
        exit(1);
    }

    if ( dwAction == ACTION_DELETE_USER )
    {
        if ( LW_IS_NULL_OR_EMPTY_STR(pszName) && !uid )
        {
            fprintf(stderr, "Please specify name or UID.\n");
            ShowUsage(GetProgramName(argv[0]));
            exit(1);
        }
    }

    if ( dwAction == ACTION_DELETE_GROUP )
    {
        if ( LW_IS_NULL_OR_EMPTY_STR(pszName) && !gid )
        {
            fprintf(stderr, "Please specify name or GID.\n");
            ShowUsage(GetProgramName(argv[0]));
            exit(1);
        }
    }

    *pdwAction = dwAction;
    *ppszName = pszName;
    *pUID = uid;
    *pGID = gid;
    *pdwInfoLevel = dwInfoLevel;
    *pdwBatchSize = dwBatchSize;

cleanup:

    return dwError;

error:

    *pdwAction = ACTION_NONE;
    *ppszName = NULL;
    *pUID = 0;
    *pGID = 0;
    *pdwBatchSize = 0;

    LW_SAFE_FREE_STRING(pszName);

    goto cleanup;
}

static
VOID
ShowUsage(
    PCSTR pszProgramName
    )
{
    fprintf(stdout, "Usage: %s --delete-all\n", pszProgramName);
    fprintf(stdout, "       %s --delete-user {--name <user login id> | --uid <uid>} \n", pszProgramName);
    fprintf(stdout, "       %s --delete-group {--name <group name> | --gid <gid>} \n", pszProgramName);
    fprintf(stdout, "       %s --enum-users {--level [0, 1, 2]} {--batchsize [1..1000]}\n", pszProgramName);
    fprintf(stdout, "       %s --enum-groups {--level [0, 1]} {--batchsize [1..1000]}\n\n", pszProgramName);
    fprintf(stdout, "\t--delete-all        Deletes everything from the cache\n");
    fprintf(stdout, "\t--delete-user       Deletes one user from the cache\n");
    fprintf(stdout, "\t--delete-group      Deletes one group from the cache\n");
    fprintf(stdout, "\t--enum-users        Enumerates users in the cache\n");
    fprintf(stdout, "\t--enum-groups       Enumerates groups in the cache\n\n");
}

static
DWORD
EnumerateUsers(
    HANDLE hLsaConnection,
    DWORD  dwUserInfoLevel,
    DWORD  dwBatchSize
    )
{
    DWORD dwError = 0;

    PSTR   pszResume = NULL;
    PVOID* ppUserInfoList = NULL;
    DWORD  dwNumUsersFound = 0;
    DWORD  dwTotalUsersFound = 0;

    do
    {
        DWORD iUser = 0;

        if (ppUserInfoList) {
           LsaFreeUserInfoList(dwUserInfoLevel, ppUserInfoList, dwNumUsersFound);
           ppUserInfoList = NULL;
        }

        dwError = LsaAdEnumUsersFromCache(
                    hLsaConnection,
                    &pszResume,
                    dwUserInfoLevel,
                    dwBatchSize,
                    &dwNumUsersFound,
                    &ppUserInfoList);
        BAIL_ON_LSA_ERROR(dwError);

        if (!dwNumUsersFound) {
            break;
        }

        dwTotalUsersFound+=dwNumUsersFound;

        for (iUser = 0; iUser < dwNumUsersFound; iUser++)
        {
            PVOID pUserInfo = *(ppUserInfoList + iUser);

            switch(dwUserInfoLevel)
            {
                case 0:
                    PrintUserInfo_0((PLSA_USER_INFO_0)pUserInfo);
                    break;
                case 1:
                    PrintUserInfo_1((PLSA_USER_INFO_1)pUserInfo);
                    break;
                case 2:
                    PrintUserInfo_2((PLSA_USER_INFO_2)pUserInfo);
                    break;
                default:

                    fprintf(stderr,
                            "Error: Invalid user info level [%d]\n",
                            dwUserInfoLevel);
                    break;
            }
        }
    } while (pszResume);

    fprintf(stdout, "TotalNumUsersFound:      %u\n", dwTotalUsersFound);

cleanup:

    if (ppUserInfoList) {
       LsaFreeUserInfoList(dwUserInfoLevel, ppUserInfoList, dwNumUsersFound);
    }

    LW_SAFE_FREE_STRING(pszResume);

    return (dwError);

error:

    goto cleanup;
}

static
DWORD
EnumerateGroups(
    HANDLE hLsaConnection,
    DWORD  dwGroupInfoLevel,
    DWORD  dwBatchSize
    )
{
    DWORD dwError = 0;

    PSTR   pszResume = NULL;
    PVOID* ppGroupInfoList = NULL;
    DWORD  dwNumGroupsFound = 0;
    DWORD  dwTotalGroupsFound = 0;

    do
    {
        DWORD iGroup = 0;

        if (ppGroupInfoList) {
           LsaFreeGroupInfoList(dwGroupInfoLevel, ppGroupInfoList, dwNumGroupsFound);
           ppGroupInfoList = NULL;
        }

        dwError = LsaAdEnumGroupsFromCache(
                    hLsaConnection,
                    &pszResume,
                    dwGroupInfoLevel,
                    dwBatchSize,
                    &dwNumGroupsFound,
                    &ppGroupInfoList);
        BAIL_ON_LSA_ERROR(dwError);

        if (!dwNumGroupsFound) {
            break;
        }

        dwTotalGroupsFound+=dwNumGroupsFound;

        for (iGroup = 0; iGroup < dwNumGroupsFound; iGroup++)
        {
            PVOID pGroupInfo = *(ppGroupInfoList + iGroup);

            switch(dwGroupInfoLevel)
            {
                case 0:
                    PrintGroupInfo_0((PLSA_GROUP_INFO_0)pGroupInfo);
                    break;
                case 1:
                    PrintGroupInfo_1((PLSA_GROUP_INFO_1)pGroupInfo);
                    break;
                default:

                    fprintf(stderr,
                            "Error: Invalid Group info level [%d]\n",
                            dwGroupInfoLevel);
                    break;
            }
        }
    } while (pszResume);

    fprintf(stdout, "TotalNumGroupsFound:      %d\n", dwTotalGroupsFound);

cleanup:

    if (ppGroupInfoList) {
       LsaFreeGroupInfoList(dwGroupInfoLevel, ppGroupInfoList, dwNumGroupsFound);
    }

    LW_SAFE_FREE_STRING(pszResume);

    return (dwError);

error:

    goto cleanup;
}

static
VOID
PrintUserInfo_0(
    PLSA_USER_INFO_0 pUserInfo
    )
{
    fprintf(stdout, "User info (Level-0):\n");
    fprintf(stdout, "====================\n");
    fprintf(stdout, "Name:     %s\n",
            LW_IS_NULL_OR_EMPTY_STR(pUserInfo->pszName) ? "<null>" : pUserInfo->pszName);
    fprintf(stdout, "Uid:      %u\n", (unsigned int)pUserInfo->uid);
    fprintf(stdout, "Gid:      %u\n", (unsigned int)pUserInfo->gid);
    fprintf(stdout, "Gecos:    %s\n",
            LW_IS_NULL_OR_EMPTY_STR(pUserInfo->pszGecos) ? "<null>" : pUserInfo->pszGecos);
    fprintf(stdout, "Shell:    %s\n",
            LW_IS_NULL_OR_EMPTY_STR(pUserInfo->pszShell) ? "<null>" : pUserInfo->pszShell);
    fprintf(stdout, "Home dir: %s\n",
            LW_IS_NULL_OR_EMPTY_STR(pUserInfo->pszHomedir) ? "<null>" : pUserInfo->pszHomedir);
    fprintf(stdout, "\n");
}

static
VOID
PrintUserInfo_1(
    PLSA_USER_INFO_1 pUserInfo
    )
{
    fprintf(stdout, "User info (Level-1):\n");
    fprintf(stdout, "====================\n");
    fprintf(stdout, "Name:          %s\n",
                LW_IS_NULL_OR_EMPTY_STR(pUserInfo->pszName) ? "<null>" : pUserInfo->pszName);
    fprintf(stdout, "UPN:          %s\n",
                    LW_IS_NULL_OR_EMPTY_STR(pUserInfo->pszUPN) ? "<null>" : pUserInfo->pszUPN);
    fprintf(stdout, "Generated UPN: %s\n", pUserInfo->bIsGeneratedUPN ? "YES" : "NO");
    fprintf(stdout, "Uid:           %u\n", (unsigned int)pUserInfo->uid);
    fprintf(stdout, "Gid:           %u\n", (unsigned int)pUserInfo->gid);
    fprintf(stdout, "Gecos:         %s\n",
                LW_IS_NULL_OR_EMPTY_STR(pUserInfo->pszGecos) ? "<null>" : pUserInfo->pszGecos);
    fprintf(stdout, "Shell:         %s\n",
                LW_IS_NULL_OR_EMPTY_STR(pUserInfo->pszShell) ? "<null>" : pUserInfo->pszShell);
    fprintf(stdout, "Home dir:      %s\n",
                LW_IS_NULL_OR_EMPTY_STR(pUserInfo->pszHomedir) ? "<null>" : pUserInfo->pszHomedir);
    fprintf(stdout, "LMHash length: %d\n", pUserInfo->dwLMHashLen);
    fprintf(stdout, "NTHash length: %d\n", pUserInfo->dwNTHashLen);
    fprintf(stdout, "Local User:    %s\n", pUserInfo->bIsLocalUser ? "YES" : "NO");
    fprintf(stdout, "\n");
}

static
VOID
PrintUserInfo_2(
    PLSA_USER_INFO_2 pUserInfo
    )
{
    fprintf(stdout, "User info (Level-2):\n");
    fprintf(stdout, "====================\n");
    fprintf(stdout, "Name:                       %s\n",
                LW_IS_NULL_OR_EMPTY_STR(pUserInfo->pszName) ? "<null>" : pUserInfo->pszName);
    fprintf(stdout, "UPN:                        %s\n",
                    LW_IS_NULL_OR_EMPTY_STR(pUserInfo->pszUPN) ? "<null>" : pUserInfo->pszUPN);
    fprintf(stdout, "Generated UPN:              %s\n", pUserInfo->bIsGeneratedUPN ? "YES" : "NO");
    fprintf(stdout, "Uid:                        %u\n", (unsigned int)pUserInfo->uid);
    fprintf(stdout, "Gid:                        %u\n", (unsigned int)pUserInfo->gid);
    fprintf(stdout, "Gecos:                      %s\n",
                LW_IS_NULL_OR_EMPTY_STR(pUserInfo->pszGecos) ? "<null>" : pUserInfo->pszGecos);
    fprintf(stdout, "Shell:                      %s\n",
                LW_IS_NULL_OR_EMPTY_STR(pUserInfo->pszShell) ? "<null>" : pUserInfo->pszShell);
    fprintf(stdout, "Home dir:                   %s\n",
                LW_IS_NULL_OR_EMPTY_STR(pUserInfo->pszHomedir) ? "<null>" : pUserInfo->pszHomedir);
    fprintf(stdout, "LMHash length:              %d\n", pUserInfo->dwLMHashLen);
    fprintf(stdout, "NTHash length:              %d\n", pUserInfo->dwNTHashLen);
    fprintf(stdout, "Local User:                 %s\n", pUserInfo->bIsLocalUser ? "YES" : "NO");
    fprintf(stdout, "Account disabled:           %s\n",
            pUserInfo->bAccountDisabled ? "TRUE" : "FALSE");
    fprintf(stdout, "Account Expired:            %s\n",
            pUserInfo->bAccountExpired ? "TRUE" : "FALSE");
    fprintf(stdout, "Account Locked:             %s\n",
            pUserInfo->bAccountLocked ? "TRUE" : "FALSE");
    fprintf(stdout, "Password never expires:     %s\n",
            pUserInfo->bPasswordNeverExpires ? "TRUE" : "FALSE");
    fprintf(stdout, "Password Expired:           %s\n",
            pUserInfo->bPasswordExpired ? "TRUE" : "FALSE");
    fprintf(stdout, "Prompt for password change: %s\n",
            pUserInfo->bPromptPasswordChange ? "YES" : "NO");
    fprintf(stdout, "User can change password:   %s\n",
            pUserInfo->bUserCanChangePassword ? "YES" : "NO");
    fprintf(stdout, "Days till password expires: %d\n",
            pUserInfo->dwDaysToPasswordExpiry);
    fprintf(stdout, "\n");
}

static
VOID
PrintGroupInfo_0(
    PLSA_GROUP_INFO_0 pGroupInfo
    )
{
    fprintf(stdout, "Group info (Level-0):\n");
    fprintf(stdout, "====================\n");
    fprintf(stdout, "Name:     %s\n",
                LW_IS_NULL_OR_EMPTY_STR(pGroupInfo->pszName) ? "<null>" : pGroupInfo->pszName);
    fprintf(stdout, "Gid:      %u\n", (unsigned int)pGroupInfo->gid);
    fprintf(stdout, "SID:      %s\n",
                    LW_IS_NULL_OR_EMPTY_STR(pGroupInfo->pszSid) ? "<null>" : pGroupInfo->pszSid);
    fprintf(stdout, "\n");
}

static
VOID
PrintGroupInfo_1(
    PLSA_GROUP_INFO_1 pGroupInfo
    )
{
    PSTR* ppszMembers = NULL;
    DWORD iMember = 0;

    fprintf(stdout, "Group info (Level-1):\n");
    fprintf(stdout, "====================\n");
    fprintf(stdout, "Name:     %s\n",
            LW_IS_NULL_OR_EMPTY_STR(pGroupInfo->pszName) ? "<null>" : pGroupInfo->pszName);
    fprintf(stdout, "Gid:      %u\n", (unsigned int)pGroupInfo->gid);
    fprintf(stdout, "SID:      %s\n",
                        LW_IS_NULL_OR_EMPTY_STR(pGroupInfo->pszSid) ? "<null>" : pGroupInfo->pszSid);
    fprintf(stdout, "Members:");

    ppszMembers = pGroupInfo->ppszMembers;

    if (ppszMembers){
        while (!LW_IS_NULL_OR_EMPTY_STR(*ppszMembers)) {
            if (iMember) {
                fprintf(stdout, "\n          %s", *ppszMembers);
            }
            else
            {
                fprintf(stdout, "  %s", *ppszMembers);
            }
            iMember++;
            ppszMembers++;
        }
        fprintf(stdout, "\n");
    }
    else
    {
        fprintf(stdout, "\n");
    }
    fprintf(stdout, "\n");
}

static
DWORD
MapErrorCode(
    DWORD dwError
    )
{
    DWORD dwError2 = dwError;

    switch (dwError)
    {
        case ECONNREFUSED:
        case ENETUNREACH:
        case ETIMEDOUT:

            dwError2 = LW_ERROR_LSA_SERVER_UNREACHABLE;

            break;

        default:

            break;
    }

    return dwError2;
}

static
BOOLEAN
IsUnsignedInteger(
    PCSTR pszIntegerCandidate
    )
{
    typedef enum {
        PARSE_MODE_LEADING_SPACE = 0,
        PARSE_MODE_INTEGER,
        PARSE_MODE_TRAILING_SPACE
    } ParseMode;

    ParseMode parseMode = PARSE_MODE_LEADING_SPACE;
    BOOLEAN bIsUnsignedInteger = TRUE;
    INT iLength = 0;
    INT iCharIdx = 0;
    CHAR cNext = '\0';

    if (LW_IS_NULL_OR_EMPTY_STR(pszIntegerCandidate))
    {
        bIsUnsignedInteger = FALSE;
        goto error;
    }

    iLength = strlen(pszIntegerCandidate);

    do {

      cNext = pszIntegerCandidate[iCharIdx++];

      switch(parseMode) {

          case PARSE_MODE_LEADING_SPACE:
          {
              if (isdigit((int)cNext))
              {
                  parseMode = PARSE_MODE_INTEGER;
              }
              else if (!isspace((int)cNext))
              {
                  bIsUnsignedInteger = FALSE;
              }
              break;
          }

          case PARSE_MODE_INTEGER:
          {
              if (isspace((int)cNext))
              {
                  parseMode = PARSE_MODE_TRAILING_SPACE;
              }
              else if (!isdigit((int)cNext))
              {
                  bIsUnsignedInteger = FALSE;
              }
              break;
          }

          case PARSE_MODE_TRAILING_SPACE:
          {
              if (!isspace((int)cNext))
              {
                  bIsUnsignedInteger = FALSE;
              }
              break;
          }
      }

    } while (iCharIdx < iLength && bIsUnsignedInteger == TRUE);


error:

    return bIsUnsignedInteger;
}

