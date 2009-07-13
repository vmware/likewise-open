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
 *        del_group.c
 *
 * Abstract:
 *
 *        Likewise Security and Authentication Subsystem (LSASS)
 *        Main file to delete a group
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 */

#include "del_group.h"

DWORD
MapErrorCode(
    DWORD dwError
    );

BOOLEAN
IsUnsignedInteger(
    PCSTR pszIntegerCandidate
    );

static
PSTR
LsaGetProgramName(
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
void
ShowUsage(
    PCSTR pszProgramName
    )
{
    printf("Usage: %s [ --gid  id ] group\n", pszProgramName);
}

static
DWORD
ParseArgs(
    int argc,
    PSTR argv[],
    PSTR* ppszGid,
    PSTR* ppszGroup
    )
{
    typedef enum {
        PARSE_MODE_OPEN = 0,
        PARSE_MODE_GID,
        PARSE_MODE_DONE
    } ParseMode;

    DWORD dwError = 0;
    ParseMode parseMode = PARSE_MODE_OPEN;
    int iArg = 1;
    PSTR pArg = NULL;
    PSTR pszGid = NULL;
    PSTR pszGroup = NULL;
    
    do {

      pArg = argv[iArg++];
      
      if (pArg == NULL || *pArg == '\0') {
        break;
      }
      
      switch(parseMode) {

          case PARSE_MODE_OPEN:
          {
              if ((strcmp(pArg, "--help") == 0) ||
                     (strcmp(pArg, "-h") == 0)) {
                ShowUsage(LsaGetProgramName(argv[0]));
                exit(0);
              }
              else if (strcmp(pArg, "--gid") == 0) {
                parseMode = PARSE_MODE_GID;
              }
              else {
                  dwError = LsaAllocateString(pArg, &pszGroup);
                  BAIL_ON_LSA_ERROR(dwError);
                  parseMode = PARSE_MODE_DONE;
              }
              
              break;
          }
    
          case PARSE_MODE_GID:
          {          
              if (!IsUnsignedInteger(pArg))
              {
                fprintf(stderr, "Please specifiy a gid that is an unsigned integer\n");
                ShowUsage(LsaGetProgramName(argv[0]));
                exit(1);
              }         
              
              dwError = LsaAllocateString(pArg, &pszGid);
              BAIL_ON_LSA_ERROR(dwError);
    
              parseMode = PARSE_MODE_OPEN;
    
              break;
          }
            
          case PARSE_MODE_DONE:
          {
              ShowUsage(LsaGetProgramName(argv[0]));
              exit(1);
          }
      }

    } while (iArg < argc);

    if (parseMode != PARSE_MODE_OPEN && parseMode != PARSE_MODE_DONE)
    {
        ShowUsage(LsaGetProgramName(argv[0]));;
        exit(1);  
    }
    
    *ppszGid = pszGid;
    *ppszGroup = pszGroup;
    
cleanup:

    return dwError;

error:

    LSA_SAFE_FREE_STRING(pszGid);
    LSA_SAFE_FREE_STRING(pszGroup);

    *ppszGid = NULL;
    *ppszGroup = NULL;
    
    goto cleanup;
}

DWORD
LsaDelGroupMain(
    int argc,
    char* argv[]
    )
{
    DWORD dwError = 0;
    HANDLE hLsaConnection = (HANDLE)NULL;
    size_t dwErrorBufferSize = 0;
    BOOLEAN bPrintOrigError = TRUE;
    PSTR pszGid = NULL;
    PSTR pszGroup = NULL;
    
    dwError = ParseArgs(
                    argc,
                    argv,
                    &pszGid,
                    &pszGroup);
    BAIL_ON_LSA_ERROR(dwError);
    
    dwError = LsaOpenServer(&hLsaConnection);
    BAIL_ON_LSA_ERROR(dwError);
    

    if (!IsNullOrEmptyString(pszGid))
    {
        gid_t gid = 0;
        sscanf(pszGid, "%u", (unsigned int*)&gid);
        
        dwError = LsaDeleteGroupById(
                        hLsaConnection,
                        gid);
        BAIL_ON_LSA_ERROR(dwError);
    }
    else if (!IsNullOrEmptyString(pszGroup))
    {
        dwError = LsaDeleteGroupByName(
                        hLsaConnection,
                        pszGroup);
        BAIL_ON_LSA_ERROR(dwError);
    }
    else
    {
        ShowUsage(LsaGetProgramName(argv[0]));
        exit(1);
    }
    
    fprintf(stdout, "The group has been deleted successfully.\n");

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
        
        dwError2 = LsaAllocateMemory(
                    dwErrorBufferSize,
                    (PVOID*)&pszErrorBuffer);
        
        if (!dwError2)
        {
            DWORD dwLen = LwGetErrorString(dwError, pszErrorBuffer, dwErrorBufferSize);
            
            if ((dwLen == dwErrorBufferSize) && !IsNullOrEmptyString(pszErrorBuffer))
            {
                fprintf(stderr, "Failed to delete group.  %s\n", pszErrorBuffer);
                bPrintOrigError = FALSE;
            }
        }
        
        LSA_SAFE_FREE_STRING(pszErrorBuffer);
    }
    
    if (bPrintOrigError)
    {
        fprintf(stderr, "Failed to delete group. Error code [%d]\n", dwError);
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
            
            dwError2 = LW_ERROR_LSA_SERVER_UNREACHABLE;
            
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
