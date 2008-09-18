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
 *        Program to list groups for user
 *
 * Authors: Sriram Nambakam (snambakam@likewisesoftware.com)
 *
 */

#define _POSIX_PTHREAD_SEMANTICS 1

#include "config.h"
#include "lsasystem.h"
#include "lsadef.h"
#include "lsa/lsa.h"

#include "lsaclient.h"
#include "lsaipc.h"

static
void
ShowUsage()
{
    printf("Usage: lw-list-groups <user name>\n");
}

DWORD
MapErrorCode(
    DWORD dwError
    );

static
DWORD
ParseArgs(
    int argc,
    char* argv[],
    PSTR* ppszUserName
    )
{
    DWORD dwError = 0;
    int iArg = 1;
    PSTR pszArg = NULL;
    PSTR pszUserName = NULL;   
    size_t dwErrorBufferSize = 0;
    BOOLEAN bPrintOrigError = TRUE;
    
    if (argc != 2)
    {
        ShowUsage();
        exit(1);
    }
    
    do {
        pszArg = argv[iArg++];
        if (pszArg == NULL || *pszArg == '\0')
        {
            break;
        }
        
        if ((strcmp(pszArg, "--help") == 0) || (strcmp(pszArg, "-h") == 0))
        {
            ShowUsage();
            exit(0);
        }
        else
        {
            dwError = LsaAllocateString(pszArg, &pszUserName);
            BAIL_ON_LSA_ERROR(dwError);
            break;
        }
    } while (iArg < argc);

    if (IsNullOrEmptyString(pszUserName)) {
       fprintf(stderr, "Please specify a user name to query for.\n");
       ShowUsage();
       exit(1);
    }

    *ppszUserName = pszUserName;

cleanup:
    
    return dwError;

error:

    *ppszUserName = NULL;

    LSA_SAFE_FREE_STRING(pszUserName);

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
                fprintf(stderr, "Failed to list groups for user.  %s\n", pszErrorBuffer);
                bPrintOrigError = FALSE;
            }
        }
        
        LSA_SAFE_FREE_STRING(pszErrorBuffer);
    }
    
    if (bPrintOrigError)
    {
        fprintf(stderr, "Failed to list groups for user. Error code [%d]\n", dwError);
    }
    
    goto cleanup;
}

int
main(
    int argc,
    char* argv[]
    )
{
    DWORD dwError = 0;
    HANDLE hLsaConnection = (HANDLE)NULL;
    PSTR   pszUserName = NULL;    
    gid_t* pGid = NULL;
    DWORD  dwNumGroups = 0;
    DWORD  iGroup = 0;

    dwError = ParseArgs(argc, argv, &pszUserName);
    BAIL_ON_LSA_ERROR(dwError);
    
    dwError = LsaOpenServer(&hLsaConnection);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaGetGroupsForUserName(
                  hLsaConnection,
                  pszUserName,
                  &dwNumGroups,
                  &pGid);
    BAIL_ON_LSA_ERROR(dwError);
    
    printf("Number of groups found for user [%s] : %d\n", pszUserName, dwNumGroups);
    
    for (iGroup = 0; iGroup < dwNumGroups; iGroup++)
    {
        fprintf(stdout,
                "Group[%d of %d] (gid = %u)\n",
                iGroup+1,
                dwNumGroups,
                (unsigned int)*(pGid+iGroup));
    }

cleanup:

    LSA_SAFE_FREE_STRING(pszUserName);    
    LSA_SAFE_FREE_MEMORY(pGid);

    return (dwError);

error:

    fprintf(stderr,
            "Failed to find groups for user (%s). Error code: %d\n",
            IsNullOrEmptyString(pszUserName) ? "<null>" : pszUserName,
            dwError); 

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
