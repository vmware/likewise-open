/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

/*
 * Copyright Likewise Software    2004-2008
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
 *
 *        Tool to enumerate NSS Maps
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 */

#define _POSIX_PTHREAD_SEMANTICS 1

#include "config.h"
#include "lsasystem.h"
#include "lsadef.h"
#include "lsa/lsa.h"

#include "lsaclient.h"
#include "lsaipc.h"

#define YPCAT_SAFE_LOG_STRING(x) \
    ( (x) ? (x) : "" )

static
DWORD
ParseArgs(
    int      argc,
    char*    argv[],
    PSTR*    ppszMapName,
    PSTR*    ppszDomain,
    PBOOLEAN pbPrintKeys,
    PBOOLEAN pbPrintNicknameTable,
    PBOOLEAN pbUseNicknameTable
    );

static
VOID
ShowUsage();

static
DWORD
EnumerateUsers(
    HANDLE  hLsaConnection,
    BOOLEAN bPrintKeys
    );

static
DWORD
EnumerateGroups(
    HANDLE  hLsaConnection,
    BOOLEAN bPrintKeys
    );

static
DWORD
EnumerateMaps(
    HANDLE  hLsaConnection,
    PCSTR   pszMapName,
    BOOLEAN bPrintKeys
    );

static
VOID
PrintUserInfo_0(
    PLSA_USER_INFO_0 pUserInfo,
    BOOLEAN bPrintKeys
    );

static
VOID
PrintGroupInfo_1(
    PLSA_GROUP_INFO_1 pGroupInfo,
    BOOLEAN bPrintKeys
    );

static
VOID
PrintMapInfo_0(
    PLSA_NSS_ARTEFACT_INFO_0 pMapInfo,
    BOOLEAN bPrintKeysOnly
    );

static
VOID
PrintNicknameTable(
    PDLINKEDLIST pNicknameList
    );

static
DWORD
MapErrorCode(
    DWORD dwError
    );

int
main(
    int argc,
    char* argv[]
    )
{
    DWORD dwError = 0;
    HANDLE hLsaConnection = (HANDLE)NULL;
    size_t dwErrorBufferSize = 0;
    BOOLEAN bPrintOrigError = TRUE;
    PSTR    pszMapName = NULL;
    PSTR    pszDomain = NULL;
    BOOLEAN bPrintKeys = FALSE;
    BOOLEAN bPrintNicknameTable = FALSE;
    BOOLEAN bUseNicknameTable = TRUE;
    PDLINKEDLIST pNISNicknameList = NULL;
    BOOLEAN bNoNicknameFile = FALSE;
    PCSTR   pszNicknameFilePath = "/var/yp/nicknames";

    dwError = ParseArgs(
                    argc,
                    argv,
                    &pszMapName,
                    &pszDomain,
                    &bPrintKeys,
                    &bPrintNicknameTable,
                    &bUseNicknameTable);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaNISGetNicknames(
                    pszNicknameFilePath,
                    &pNISNicknameList);
    if (dwError == ENOENT)
    {
        bNoNicknameFile = TRUE;
        dwError = 0;
    }

    if (bPrintNicknameTable)
    {
        if (bNoNicknameFile)
        {
            printf("nickname file %s does not exist.\n", pszNicknameFilePath);
            goto cleanup;
        }

        if (pNISNicknameList)
        {
            PrintNicknameTable(pNISNicknameList);
        }

        goto cleanup;
    }

    if (bUseNicknameTable)
    {
        PCSTR pszLookupName = NULL;

        if (bNoNicknameFile)
        {
            printf("nickname file %s does not exist.\n", pszNicknameFilePath);
        }

        if (pNISNicknameList)
        {
            pszLookupName = LsaNISLookupAlias(
                                pNISNicknameList,
                                pszMapName);

            if (pszLookupName)
            {
                LSA_SAFE_FREE_STRING(pszMapName);

                dwError = LsaAllocateString(
                                pszLookupName,
                                &pszMapName);
                BAIL_ON_LSA_ERROR(dwError);
            }
        }
    }

    dwError = LsaOpenServer(&hLsaConnection);
    BAIL_ON_LSA_ERROR(dwError);

    if (!strcasecmp(pszMapName, "passwd.byname") ||
        !strcasecmp(pszMapName, "passwd"))
    {
        dwError = EnumerateUsers(hLsaConnection, bPrintKeys);
    }
    else if (!strcasecmp(pszMapName, "group.byname") ||
             !strcasecmp(pszMapName, "group"))
    {
        dwError = EnumerateGroups(hLsaConnection, bPrintKeys);
    }
    else
    {
        dwError = EnumerateMaps(
                        hLsaConnection,
                        pszMapName,
                        bPrintKeys);
    }
    BAIL_ON_LSA_ERROR(dwError);

cleanup:

    if (hLsaConnection != (HANDLE)NULL) {
        LsaCloseServer(hLsaConnection);
    }

    if (pNISNicknameList)
    {
        LsaNISFreeNicknameList(pNISNicknameList);
    }

    LSA_SAFE_FREE_STRING(pszMapName);
    LSA_SAFE_FREE_STRING(pszDomain);

    return (dwError);

error:

    dwError = MapErrorCode(dwError);

    dwErrorBufferSize = LsaGetErrorString(dwError, NULL, 0);

    if (dwErrorBufferSize > 0)
    {
        DWORD dwError2 = 0;
        PSTR   pszErrorBuffer = NULL;

        dwError2 = LsaAllocateMemory(
                    dwErrorBufferSize,
                    (PVOID*)&pszErrorBuffer);

        if (!dwError2)
        {
            DWORD dwLen = LsaGetErrorString(dwError, pszErrorBuffer, dwErrorBufferSize);

            if ((dwLen == dwErrorBufferSize) && !IsNullOrEmptyString(pszErrorBuffer))
            {
                fprintf(stderr, "Failed to enumerate maps.  %s\n", pszErrorBuffer);
                bPrintOrigError = FALSE;
            }
        }

        LSA_SAFE_FREE_STRING(pszErrorBuffer);
    }

    if (bPrintOrigError)
    {
        fprintf(stderr, "Failed to enumerate maps. Error code [%d]\n", dwError);
    }

    dwError = 1;

    goto cleanup;
}

static
DWORD
ParseArgs(
    int            argc,
    char*          argv[],
    PSTR*          ppszMapName,
    PSTR*          ppszDomain,
    PBOOLEAN       pbPrintKeys,
    PBOOLEAN       pbPrintNicknameTable,
    PBOOLEAN       pbUseNicknameTable
    )
{
    typedef enum {
            PARSE_MODE_OPEN = 0,
            PARSE_MODE_DOMAIN,
            PARSE_MODE_DONE
        } ParseMode;

    DWORD dwError = 0;
    int iArg = 1;
    PSTR pszArg = NULL;
    ParseMode parseMode = PARSE_MODE_OPEN;
    PSTR pszMapName = NULL;
    BOOLEAN bPrintKeys = FALSE;
    BOOLEAN bUseNicknameTable = TRUE;
    BOOLEAN bPrintNicknameTable = FALSE;
    PSTR    pszDomain = NULL;

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
                    ShowUsage();
                    exit(0);
                }
                else if (!strcmp(pszArg, "-d")) {
                    parseMode = PARSE_MODE_DOMAIN;
                }
                else if (!strcmp(pszArg, "-k"))
                {
                    bPrintKeys = TRUE;
                }
                else if (!strcmp(pszArg, "-t"))
                {
                    bUseNicknameTable = FALSE;
                }
                else if (!strcmp(pszArg, "-x"))
                {
                    bPrintNicknameTable = TRUE;
                }
                else
                {
                    LSA_SAFE_FREE_STRING(pszMapName);

                    dwError = LsaAllocateString(
                                    pszArg,
                                    &pszMapName);
                    BAIL_ON_LSA_ERROR(dwError);
                }
                break;

            case PARSE_MODE_DOMAIN:

                LSA_SAFE_FREE_STRING(pszDomain);

                dwError = LsaAllocateString(
                              pszArg,
                              &pszDomain);
                BAIL_ON_LSA_ERROR(dwError);

                parseMode = PARSE_MODE_OPEN;

                break;

            case PARSE_MODE_DONE:
                ShowUsage();
                exit(1);
        }

    } while (iArg < argc);

    if (parseMode != PARSE_MODE_OPEN && parseMode != PARSE_MODE_DONE)
    {
        ShowUsage();
        exit(1);
    }

    if (!bPrintNicknameTable && IsNullOrEmptyString(pszMapName))
    {
        ShowUsage();
        exit(1);
    }

    *ppszMapName = pszMapName;
    *pbPrintKeys = bPrintKeys;
    *ppszDomain = pszDomain;
    *pbPrintNicknameTable = bPrintNicknameTable;
    *pbUseNicknameTable = bUseNicknameTable;

cleanup:

    return dwError;

error:

    LSA_SAFE_FREE_STRING(pszMapName);
    LSA_SAFE_FREE_STRING(pszDomain);

    goto cleanup;
}

static
void
ShowUsage()
{
    printf("Usage: lw-ypcat {-d <domain>} {-x} {-t} {-k} <map name>\n");
    printf("\n");
    printf("-k : query keys only.\n");
    printf("-x : print nis nickname table.\n");
    printf("-t : do not use nickname table.\n");
}

static
DWORD
EnumerateUsers(
    HANDLE  hLsaConnection,
    BOOLEAN bPrintKeys
    )
{
    DWORD  dwError = 0;
    DWORD  dwUserInfoLevel = 0;
    DWORD  dwBatchSize = 100;
    DWORD  dwNumUsersFound = 0;
    PVOID* ppUserInfoList = NULL;
    HANDLE hResume = (HANDLE)NULL;

    dwError = LsaBeginEnumUsers(
                    hLsaConnection,
                    dwUserInfoLevel,
                    dwBatchSize,
                    &hResume);
    BAIL_ON_LSA_ERROR(dwError);

    do
    {
        DWORD iUser = 0;

        if (ppUserInfoList) {
           LsaFreeUserInfoList(dwUserInfoLevel, ppUserInfoList, dwNumUsersFound);
           ppUserInfoList = NULL;
        }

        dwError = LsaEnumUsers(
                    hLsaConnection,
                    hResume,
                    &dwNumUsersFound,
                    &ppUserInfoList);
        BAIL_ON_LSA_ERROR(dwError);

        if (!dwNumUsersFound) {
            break;
        }

        for (iUser = 0; iUser < dwNumUsersFound; iUser++)
        {
            PLSA_USER_INFO_0 pUserInfo = (PLSA_USER_INFO_0)*(ppUserInfoList + iUser);

            PrintUserInfo_0(pUserInfo, bPrintKeys);
        }

    } while (dwNumUsersFound);

cleanup:

    if (ppUserInfoList) {
       LsaFreeUserInfoList(dwUserInfoLevel, ppUserInfoList, dwNumUsersFound);
    }

    if ((hResume != (HANDLE)NULL) &&
        (hLsaConnection != (HANDLE)NULL)) {
        LsaEndEnumUsers(hLsaConnection, hResume);
    }

    return dwError;

error:

    goto cleanup;
}

static
DWORD
EnumerateGroups(
    HANDLE  hLsaConnection,
    BOOLEAN bPrintKeys
    )
{
    DWORD  dwError = 0;
    DWORD  dwGroupInfoLevel = 1;
    DWORD  dwBatchSize = 100;
    DWORD  dwNumGroupsFound = 0;
    PVOID* ppGroupInfoList = NULL;
    HANDLE hResume = (HANDLE)NULL;

    dwError = LsaBeginEnumGroups(
                    hLsaConnection,
                    dwGroupInfoLevel,
                    dwBatchSize,
                    &hResume);
    BAIL_ON_LSA_ERROR(dwError);

    do
    {
        DWORD iGroup = 0;

        if (ppGroupInfoList) {
           LsaFreeGroupInfoList(dwGroupInfoLevel, ppGroupInfoList, dwNumGroupsFound);
           ppGroupInfoList = NULL;
        }

        dwError = LsaEnumGroups(
                    hLsaConnection,
                    hResume,
                    &dwNumGroupsFound,
                    &ppGroupInfoList);
        BAIL_ON_LSA_ERROR(dwError);

        if (!dwNumGroupsFound) {
            break;
        }

        for (iGroup = 0; iGroup < dwNumGroupsFound; iGroup++)
        {
            PLSA_GROUP_INFO_1 pGroupInfo = (PLSA_GROUP_INFO_1)*(ppGroupInfoList + iGroup);

            PrintGroupInfo_1(pGroupInfo, bPrintKeys);
        }

    } while (dwNumGroupsFound);

cleanup:

    if (ppGroupInfoList) {
       LsaFreeGroupInfoList(dwGroupInfoLevel, ppGroupInfoList, dwNumGroupsFound);
    }

    if ((hResume != (HANDLE)NULL) &&
        (hLsaConnection != (HANDLE)NULL)) {
        LsaEndEnumGroups(hLsaConnection, hResume);
    }

    return dwError;

error:

    goto cleanup;
}

static
DWORD
EnumerateMaps(
    HANDLE  hLsaConnection,
    PCSTR   pszMapName,
    BOOLEAN bPrintKeys
    )
{
    DWORD dwError = 0;
    DWORD dwMapInfoLevel = 0;
    DWORD dwBatchSize = 100;
    DWORD  dwNumMapsFound = 0;
    DWORD  dwTotalMapsFound = 0;
    LSA_NIS_MAP_QUERY_FLAGS dwFlags = LSA_NIS_MAP_QUERY_ALL;
    PVOID* ppMapInfoList = NULL;
    HANDLE hResume = (HANDLE)NULL;

    dwError = LsaBeginEnumNSSArtefacts(
                    hLsaConnection,
                    dwMapInfoLevel,
                    pszMapName,
                    dwFlags,
                    dwBatchSize,
                    &hResume);
    BAIL_ON_LSA_ERROR(dwError);

    do
    {
        DWORD iMap = 0;

        dwError = LsaEnumNSSArtefacts(
                    hLsaConnection,
                    hResume,
                    &dwNumMapsFound,
                    &ppMapInfoList);
        BAIL_ON_LSA_ERROR(dwError);

        if (!dwNumMapsFound) {
            break;
        }

        dwTotalMapsFound += dwNumMapsFound;

        for (iMap = 0; iMap < dwNumMapsFound; iMap++)
        {
            PVOID pMapInfo = *(ppMapInfoList + iMap);

            switch(dwMapInfoLevel)
            {
                case 0:

                    PrintMapInfo_0((PLSA_NSS_ARTEFACT_INFO_0)pMapInfo, bPrintKeys);
                    break;

                default:

                    fprintf(stderr,
                            "Error: Invalid map info level [%d]\n",
                            dwMapInfoLevel);
                    break;
            }
        }
    } while (dwNumMapsFound);

cleanup:

    if (ppMapInfoList) {
       LsaFreeNSSArtefactInfoList(dwMapInfoLevel, ppMapInfoList, dwNumMapsFound);
    }

    if ((hResume != (HANDLE)NULL) &&
        (hLsaConnection != (HANDLE)NULL)) {
        LsaEndEnumNSSArtefacts(hLsaConnection, hResume);
    }

    return dwError;

error:

    goto cleanup;
}

static
VOID
PrintUserInfo_0(
    PLSA_USER_INFO_0 pUserInfo,
    BOOLEAN bPrintKeys
    )
{
    if (bPrintKeys)
    {
        printf("%s ", pUserInfo->pszName);
    }

    printf("%s:%s:%u:%u:%s:%s:%s\n",
           YPCAT_SAFE_LOG_STRING(pUserInfo->pszName),
           YPCAT_SAFE_LOG_STRING(pUserInfo->pszPasswd),
           (unsigned int)pUserInfo->uid,
           (unsigned int)pUserInfo->gid,
           YPCAT_SAFE_LOG_STRING(pUserInfo->pszGecos),
           YPCAT_SAFE_LOG_STRING(pUserInfo->pszHomedir),
           YPCAT_SAFE_LOG_STRING(pUserInfo->pszShell));
}

static
VOID
PrintGroupInfo_1(
    PLSA_GROUP_INFO_1 pGroupInfo,
    BOOLEAN bPrintKeys
    )
{
    PSTR* ppszMembers = NULL;

    if (bPrintKeys)
    {
        printf("%s ", pGroupInfo->pszName);
    }

    printf("%s:%s:%u:",
           YPCAT_SAFE_LOG_STRING(pGroupInfo->pszName),
           YPCAT_SAFE_LOG_STRING(pGroupInfo->pszPasswd),
           (unsigned int)pGroupInfo->gid);

    ppszMembers = pGroupInfo->ppszMembers;

    if (ppszMembers)
    {
        DWORD iMember = 0;

        while (!IsNullOrEmptyString(*ppszMembers))
        {
          if (iMember)
          {
             printf(",%s", *ppszMembers);
          }
          else
          {
             printf("%s", *ppszMembers);
          }
          iMember++;
          ppszMembers++;
       }
    }

    printf("\n");
}

static
VOID
PrintMapInfo_0(
    PLSA_NSS_ARTEFACT_INFO_0 pMapInfo,
    BOOLEAN bPrintKeysOnly
    )
{
    if (bPrintKeysOnly)
    {
        printf("%s\n", YPCAT_SAFE_LOG_STRING(pMapInfo->pszName));
    }
    else
    {
        printf("%s %s\n",
               YPCAT_SAFE_LOG_STRING(pMapInfo->pszName),
               YPCAT_SAFE_LOG_STRING(pMapInfo->pszValue));
    }
}

static
VOID
PrintNicknameTable(
    PDLINKEDLIST pNicknameList
    )
{
    PDLINKEDLIST pIter = pNicknameList;

    for (; pIter; pIter = pIter->pNext)
    {
        PLSA_NIS_NICKNAME pNickname = (PLSA_NIS_NICKNAME)pIter->pItem;

        printf("Use \"%s\" for map \"%s\"\n", pNickname->pszMapAlias, pNickname->pszMapName);
    }
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

            dwError2 = LSA_ERROR_LSA_SERVER_UNREACHABLE;

            break;

        default:

            break;
    }

    return dwError2;
}


