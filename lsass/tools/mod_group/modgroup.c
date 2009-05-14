/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 */

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
 *        modgroup.c
 *
 * Abstract:
 *
 *        Likewise Security and Authentication Subsystem (LSASS)
 *
 *        Driver for program to modify an existing group
 *
 * Authors:
 *          Krishna Ganugapati (krishnag@likewise.com)
 *          Sriram Nambakam (snambakam@likewise.com)
 *          Rafal Szczesniak (rafal@likewise.com)
 */

#include "includes.h"

DWORD
MapErrorCode(
    DWORD dwError
    );

DWORD
LsaModGroupMain(
    int argc,
    char* argv[]
    )
{
    DWORD dwError = 0;
    PDLINKEDLIST pTaskList = NULL;
    PSTR pszLoginId = NULL;
    size_t dwErrorBufferSize = 0;
    BOOLEAN bPrintOrigError = TRUE;

     if (geteuid() != 0) {
         fprintf(stderr, "This program requires super-user privileges.\n");
         dwError = EACCES;
         BAIL_ON_LSA_ERROR(dwError);
     }

     dwError = ParseArgs(argc, argv, &pszLoginId, &pTaskList);
     BAIL_ON_LSA_ERROR(dwError);

     dwError = ModifyGroup(
                     pszLoginId,
                     pTaskList);
     BAIL_ON_LSA_ERROR(dwError);

 cleanup:

     if (pTaskList) {
         LsaDLinkedListForEach(pTaskList, &FreeTasksInList, NULL);
         LsaDLinkedListFree(pTaskList);
     }

     LSA_SAFE_FREE_STRING(pszLoginId);

     return dwError;

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
                 fprintf(stderr, "Failed to modify group.  %s\n", pszErrorBuffer);
                 bPrintOrigError = FALSE;
             }
         }

         LSA_SAFE_FREE_STRING(pszErrorBuffer);
     }

     if (bPrintOrigError)
     {
         fprintf(stderr, "Failed to modify group. Error code [%d]\n", dwError);
     }

     goto cleanup;
}

DWORD
ParseArgs(
    int   argc,
    char* argv[],
    PSTR* ppszLoginId,
    PDLINKEDLIST* ppTaskList
    )
{
    typedef enum {
        PARSE_MODE_OPEN = 0,
        PARSE_MODE_ADD_TO_GROUPS,
        PARSE_MODE_REMOVE_FROM_GROUPS,
        PARSE_MODE_DONE
    } ParseMode;

    DWORD dwError = 0;
    ParseMode parseMode = PARSE_MODE_OPEN;
    int iArg = 1;
    PSTR pArg = NULL;
    PDLINKEDLIST pTaskList = NULL;
    PGROUP_MOD_TASK pTask = NULL;
    PSTR    pszLoginId = NULL;

    do {
        pArg = argv[iArg++];
        if (pArg == NULL || *pArg == '\0') {
          break;
        }

        switch(parseMode) {
            case PARSE_MODE_OPEN:
            {
                if ((strcmp(pArg, "--help") == 0) ||
                         (strcmp(pArg, "-h") == 0))
                {
                  ShowUsage(GetProgramName(argv[0]));
                  exit(0);
                }
                else if (!strcmp(pArg, "--add-to-groups"))
                {
                    parseMode = PARSE_MODE_ADD_TO_GROUPS;
                }
                else if (!strcmp(pArg, "--remove-from-groups"))
                {
                    parseMode = PARSE_MODE_REMOVE_FROM_GROUPS;
                }
                else
                {
                    dwError = LsaAllocateString(pArg, &pszLoginId);
                    BAIL_ON_LSA_ERROR(dwError);
                    parseMode = PARSE_MODE_DONE;
                }
                break;
            }

            case PARSE_MODE_REMOVE_FROM_GROUPS:
            {
                 dwError = LsaAllocateMemory(sizeof(GROUP_MOD_TASK), (PVOID*)&pTask);
                 BAIL_ON_LSA_ERROR(dwError);

                 pTask->taskType = GroupModTask_RemoveFromGroups;

                 dwError = LsaAllocateString(pArg, &pTask->pszData);
                 BAIL_ON_LSA_ERROR(dwError);

                 dwError = LsaDLinkedListAppend(&pTaskList, pTask);
                 BAIL_ON_LSA_ERROR(dwError);

                 parseMode = PARSE_MODE_OPEN;

                 break;
            }

            case PARSE_MODE_ADD_TO_GROUPS:
            {
                  dwError = LsaAllocateMemory(sizeof(GROUP_MOD_TASK), (PVOID*)&pTask);
                  BAIL_ON_LSA_ERROR(dwError);

                  pTask->taskType = GroupModTask_AddToGroups;

                  dwError = LsaAllocateString(pArg, &pTask->pszData);
                  BAIL_ON_LSA_ERROR(dwError);

                  dwError = LsaDLinkedListAppend(&pTaskList, pTask);
                  BAIL_ON_LSA_ERROR(dwError);

                  parseMode = PARSE_MODE_OPEN;

                 break;
            }

            case PARSE_MODE_DONE:
            {

                ShowUsage(GetProgramName(argv[0]));
                exit(1);
            }
        }

    } while (iArg < argc);

    if (parseMode != PARSE_MODE_OPEN && parseMode != PARSE_MODE_DONE)
    {
        ShowUsage(GetProgramName(argv[0]));
        exit(1);
    }

    if (!ValidateArgs(pszLoginId, pTaskList)) {
       dwError = LSA_ERROR_INVALID_PARAMETER;
       BAIL_ON_LSA_ERROR(dwError);
    }

    *ppszLoginId = pszLoginId;
    *ppTaskList = pTaskList;

cleanup:

    return dwError;

error:

    *ppTaskList = NULL;

    if (pTaskList) {
        LsaDLinkedListForEach(pTaskList, FreeTasksInList, NULL);
        LsaDLinkedListFree(pTaskList);
    }

    if (pTask) {
        FreeTask(pTask);
    }

    LSA_SAFE_FREE_STRING(pszLoginId);

    ShowUsage(GetProgramName(argv[0]));

    goto cleanup;
}

BOOLEAN
ValidateArgs(
    PCSTR        pszLoginId,
    PDLINKEDLIST pTaskList
    )
{
    BOOLEAN bValid = FALSE;

    PDLINKEDLIST pListMember = NULL;

    for (pListMember = pTaskList; pListMember; pListMember = pListMember->pNext)
    {
        PGROUP_MOD_TASK pTask = (PGROUP_MOD_TASK)pListMember->pItem;
        if (pTask) {
           switch(pTask->taskType)
           {
               default:
                   break;
           }
        }
    }

    if (IsNullOrEmptyString(pszLoginId)) {
        fprintf(stderr, "Error: A valid group id or group name must be specified.\n");
        goto cleanup;
    }

    bValid = TRUE;

cleanup:

    return bValid;
}

VOID
FreeTasksInList(
    PVOID pTask,
    PVOID pGroupData
    )
{
    if (pTask) {
       FreeTask((PGROUP_MOD_TASK)pTask);
    }
}

VOID
FreeTask(
    PGROUP_MOD_TASK pTask
    )
{
    LSA_SAFE_FREE_STRING(pTask->pszData);
    LsaFreeMemory(pTask);
}

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

VOID
ShowUsage(
    PCSTR pszProgramName
    )
{
    fprintf(stdout, "Usage: %s {modification options} ( group name | gid )\n\n", pszProgramName);

    fprintf(stdout, "\nModification options:\n");
    fprintf(stdout, "{ --help }\n");
    fprintf(stdout, "{ --add-to-groups nt4-style-group-name }\n");
    fprintf(stdout, "{ --remove-from-groups nt4-style-group-name }\n");
}

DWORD
ModifyGroup(
    PSTR pszGroupId,
    PDLINKEDLIST pTaskList
    )
{
    DWORD dwError = 0;
    PLSA_GROUP_MOD_INFO pGroupModInfo = NULL;
    gid_t gid = 0;
    int   nRead = 0;
    LSA_FIND_FLAGS dwFindFlags = LSA_FIND_FLAGS_NSS;
    PVOID pGroupInfo = NULL;
    DWORD dwGroupInfoLevel = 0;
    HANDLE hLsaConnection = (HANDLE)NULL;

    BAIL_ON_INVALID_STRING(pszGroupId);

    dwError = LsaOpenServer(&hLsaConnection);
    BAIL_ON_LSA_ERROR(dwError);

    nRead = sscanf(pszGroupId, "%u", (unsigned int*)&gid);
    if ((nRead == EOF) || (nRead == 0)) {

       dwError = LsaFindGroupByName(
                       hLsaConnection,
                       pszGroupId,
                       dwFindFlags,
                       dwGroupInfoLevel,
                       &pGroupInfo);
       BAIL_ON_LSA_ERROR(dwError);

       gid = ((PLSA_GROUP_INFO_0)pGroupInfo)->gid;

       LsaFreeGroupInfo(dwGroupInfoLevel, pGroupInfo);
       pGroupInfo = NULL;
    }

    dwError = BuildGroupModInfo(
                    gid,
                    pTaskList,
                    &pGroupModInfo);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaModifyGroup(
                    hLsaConnection,
                    pGroupModInfo);
    BAIL_ON_LSA_ERROR(dwError);

    fprintf(stdout, "Successfully modified group %s\n", pszGroupId);

cleanup:

    if (pGroupModInfo) {
        LsaFreeGroupModInfo(pGroupModInfo);
    }

    if (pGroupInfo) {
       LsaFreeGroupInfo(dwGroupInfoLevel, pGroupInfo);
    }

    if (hLsaConnection != (HANDLE)NULL) {
       LsaCloseServer(hLsaConnection);
    }

    return dwError;

error:

    goto cleanup;
}

DWORD
BuildGroupModInfo(
    gid_t        gid,
    PDLINKEDLIST pTaskList,
    PLSA_GROUP_MOD_INFO* ppGroupModInfo
    )
{
    DWORD dwError = 0;
    PDLINKEDLIST pListMember = pTaskList;
    PLSA_GROUP_MOD_INFO pGroupModInfo = NULL;

    dwError = LsaBuildGroupModInfo(gid, &pGroupModInfo);
    BAIL_ON_LSA_ERROR(dwError);

    for (; pListMember; pListMember = pListMember->pNext)
    {
        PGROUP_MOD_TASK pTask = (PGROUP_MOD_TASK)pListMember->pItem;
        switch(pTask->taskType)
        {
            case GroupModTask_AddToGroups:
            {
                 dwError = LsaModifyGroup_AddToGroups(pGroupModInfo, pTask->pszData);
                 BAIL_ON_LSA_ERROR(dwError);

                 break;
            }
            case GroupModTask_RemoveFromGroups:
            {
                 dwError = LsaModifyGroup_RemoveFromGroups(pGroupModInfo, pTask->pszData);
                 BAIL_ON_LSA_ERROR(dwError);

                 break;
            }
        }
    }

    *ppGroupModInfo = pGroupModInfo;

cleanup:

    return dwError;

error:

    *ppGroupModInfo = NULL;

    if (pGroupModInfo) {
       LsaFreeGroupModInfo(pGroupModInfo);
    }

    goto cleanup;
}

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


/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
