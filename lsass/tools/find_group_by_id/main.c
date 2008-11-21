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
 *        Test Program for exercising LsaFindGroupById
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

VOID
ParseArgs(
    int    argc,
    char*  argv[],
    gid_t* pGID,
    PDWORD pdwInfoLevel
    );

BOOLEAN
IsUnsignedInteger(
    PCSTR pszIntegerCandidate
    );

VOID
ShowUsage();

VOID
PrintGroupInfo_0(
    PLSA_GROUP_INFO_0 pGroupInfo
    );

VOID
PrintGroupInfo_1(
    PLSA_GROUP_INFO_1 pGroupInfo
    );

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
    gid_t gid = 0;
    DWORD dwInfoLevel = 0;
    HANDLE hLsaConnection = (HANDLE)NULL;
    PVOID pGroupInfo = NULL;
    size_t dwErrorBufferSize = 0;
    BOOLEAN bPrintOrigError = TRUE;
    
    ParseArgs(argc, argv, &gid, &dwInfoLevel);
    
    dwError = LsaOpenServer(&hLsaConnection);
    BAIL_ON_LSA_ERROR(dwError);
    
    dwError = LsaFindGroupById(
                    hLsaConnection,
                    gid,
                    0,
                    dwInfoLevel,
                    &pGroupInfo);
    BAIL_ON_LSA_ERROR(dwError);
    
    switch(dwInfoLevel)
    {
        case 0:
            PrintGroupInfo_0((PLSA_GROUP_INFO_0)pGroupInfo);
            break;
        case 1:
            PrintGroupInfo_1((PLSA_GROUP_INFO_1)pGroupInfo);
            break;
        default:
            
            fprintf(stderr, "Error: Invalid group info level [%d]\n", dwInfoLevel);
            break;
    }

cleanup:

    if (pGroupInfo) {
       LsaFreeGroupInfo(dwInfoLevel, pGroupInfo);
    }
    
    if (hLsaConnection != (HANDLE)NULL) {
        LsaCloseServer(hLsaConnection);
    }

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
                fprintf(stderr, "Failed to locate group.  %s\n", pszErrorBuffer);
                bPrintOrigError = FALSE;
            }
        }
        
        LSA_SAFE_FREE_STRING(pszErrorBuffer);
    }
    
    if (bPrintOrigError)
    {
        fprintf(stderr, "Failed to locate group. Error code [%d]\n", dwError);
    }

    goto cleanup;
}

VOID
ParseArgs(
    int    argc,
    char*  argv[],
    gid_t* pGID,
    PDWORD pdwInfoLevel
    )
{
    typedef enum {
            PARSE_MODE_OPEN = 0,
            PARSE_MODE_LEVEL,
            PARSE_MODE_DONE
        } ParseMode;
        
    int iArg = 1;
    PSTR pszArg = NULL;
    gid_t gid = 0;
    ParseMode parseMode = PARSE_MODE_OPEN;
    DWORD dwInfoLevel = 0;
    BOOLEAN bGIDSpecified = FALSE;

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
                else if (!strcmp(pszArg, "--level")) {
                    parseMode = PARSE_MODE_LEVEL;
                }
                
                else
                {
                    if (!IsUnsignedInteger(pszArg))
                    {
                        fprintf(stderr, "Please specifiy a gid that is a positive integer\n");
                        ShowUsage();
                        exit(1);
                    }
                    int nRead = sscanf(pszArg, "%u", (unsigned int*)&gid);
                    if ((EOF == nRead) || (nRead == 0)) {
                       fprintf(stderr, "Please specify a valid gid\n");
                       ShowUsage();
                       exit(1);
                    }
                    bGIDSpecified = TRUE;
                    parseMode = PARSE_MODE_DONE;
                }
                break;
                
            case PARSE_MODE_LEVEL:
            {
                if (!IsUnsignedInteger(pszArg))
                {
                    fprintf(stderr, "Please specifiy an info level that is a positive integer\n");
                    ShowUsage();
                    exit(1);
                }
                dwInfoLevel = atoi(pszArg);
                parseMode = PARSE_MODE_OPEN;
                break;
            }
            case PARSE_MODE_DONE:
            {
                ShowUsage();
                exit(1);
            }
        }
        
    } while (iArg < argc);

    if (parseMode != PARSE_MODE_OPEN && parseMode != PARSE_MODE_DONE)
    {
        ShowUsage();
        exit(1);  
    }
    
    if (!bGIDSpecified) {
       fprintf(stderr, "Please specify a gid to query for.\n");
       ShowUsage();
       exit(1);
    }

    *pGID = gid;
    *pdwInfoLevel = dwInfoLevel;

}

void
ShowUsage()
{
    printf("Usage: lw-find-group-by-id { --level [0, 1] } <gid>\n");
}

VOID
PrintGroupInfo_0(
    PLSA_GROUP_INFO_0 pGroupInfo
    )
{
    fprintf(stdout, "Group info (Level-0):\n");
    fprintf(stdout, "====================\n");
    fprintf(stdout, "Name:     %s\n",
                IsNullOrEmptyString(pGroupInfo->pszName) ? "<null>" : pGroupInfo->pszName);
    fprintf(stdout, "Gid:      %u\n", (unsigned int)pGroupInfo->gid);
    fprintf(stdout, "SID:     %s\n",
                    IsNullOrEmptyString(pGroupInfo->pszSid) ? "<null>" : pGroupInfo->pszSid);
}

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
            IsNullOrEmptyString(pGroupInfo->pszName) ? "<null>" : pGroupInfo->pszName);
    fprintf(stdout, "Gid:      %u\n", (unsigned int)pGroupInfo->gid);
    fprintf(stdout, "SID:     %s\n",
                        IsNullOrEmptyString(pGroupInfo->pszSid) ? "<null>" : pGroupInfo->pszSid);
    
    fprintf(stdout, "Members:\n");

    ppszMembers = pGroupInfo->ppszMembers;
    if (ppszMembers){
        while (!IsNullOrEmptyString(*ppszMembers)) {
              if (iMember) {
                 fprintf(stdout, "\n%s", *ppszMembers);
              } else {
                 fprintf(stdout, "%s", *ppszMembers);
              }
              iMember++;
              ppszMembers++;
        }
        fprintf(stdout, "\n");
    }
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
    
    if (IsNullOrEmptyString(pszIntegerCandidate))
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
