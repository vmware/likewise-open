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

static
DWORD
ParseArgs(
    int    argc,
    char*  argv[],
    PDWORD pdwInfoLevel,
    PDWORD pdwBatchSize,
    PSTR*  ppszMapName,
    PBOOLEAN pbPrintKeys
    );

static
VOID
ShowUsage();

static
VOID
PrintMapInfo_0(
    PLSA_NSS_ARTEFACT_INFO_0 pMapInfo,
    BOOLEAN bPrintKeys
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
main(
    int argc,
    char* argv[]
    )
{
    DWORD dwError = 0;
    DWORD dwMapInfoLevel = 0;
    DWORD dwBatchSize = 10;
    HANDLE hLsaConnection = (HANDLE)NULL;
    HANDLE hResume = (HANDLE)NULL;
    PVOID* ppMapInfoList = NULL;
    DWORD  dwNumMapsFound = 0;
    DWORD  dwTotalMapsFound = 0;
    size_t dwErrorBufferSize = 0;
    BOOLEAN bPrintOrigError = TRUE;
    PSTR    pszMapName = NULL;
    BOOLEAN bPrintKeys = FALSE;
    LSA_NIS_MAP_QUERY_FLAGS dwFlags = LSA_NIS_MAP_QUERY_ALL;

    dwError = ParseArgs(
                    argc,
                    argv,
                    &dwMapInfoLevel,
                    &dwBatchSize,
                    &pszMapName,
                    &bPrintKeys);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaOpenServer(&hLsaConnection);
    BAIL_ON_LSA_ERROR(dwError);

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

    if (hLsaConnection != (HANDLE)NULL) {
        LsaCloseServer(hLsaConnection);
    }

    LSA_SAFE_FREE_STRING(pszMapName);

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

    goto cleanup;
}

static
DWORD
ParseArgs(
    int            argc,
    char*          argv[],
    PDWORD         pdwInfoLevel,
    PDWORD         pdwBatchSize,
    PSTR*          ppszMapName,
    PBOOLEAN       pbPrintKeys
    )
{
    typedef enum {
            PARSE_MODE_OPEN = 0,
            PARSE_MODE_LEVEL,
            PARSE_MODE_BATCHSIZE,
            PARSE_MODE_DONE
        } ParseMode;

    DWORD dwError = 0;
    int iArg = 1;
    PSTR pszArg = NULL;
    ParseMode parseMode = PARSE_MODE_OPEN;
    DWORD dwInfoLevel = 0;
    DWORD dwBatchSize = 10;
    PSTR pszMapName = NULL;
    BOOLEAN bPrintKeys = FALSE;

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
                else if (!strcmp(pszArg, "--batchsize")) {
                    parseMode = PARSE_MODE_BATCHSIZE;
                }
                else if (!strcmp(pszArg, "-k"))
                {
                    bPrintKeys = TRUE;
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

            case PARSE_MODE_LEVEL:

                if (!IsUnsignedInteger(pszArg))
                {
                    fprintf(stderr, "please use an info level which is an unsigned integer.\n");
                }
                dwInfoLevel = atoi(pszArg);
                parseMode = PARSE_MODE_OPEN;

                break;

            case PARSE_MODE_BATCHSIZE:

                if (!IsUnsignedInteger(pszArg))
                {
                    fprintf(stderr, "please use a batch size which is an unsigned integer.\n");
                }

                dwBatchSize = atoi(pszArg);
                if ((dwBatchSize < 0) ||
                    (dwBatchSize > 1000)) {
                    ShowUsage();
                    exit(1);
                }
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

    if (IsNullOrEmptyString(pszMapName))
    {
        ShowUsage();
        exit(1);
    }

    *ppszMapName = pszMapName;
    *pdwInfoLevel = dwInfoLevel;
    *pdwBatchSize = dwBatchSize;
    *pbPrintKeys = bPrintKeys;

cleanup:

    return dwError;

error:

    LSA_SAFE_FREE_STRING(pszMapName);

    goto cleanup;
}

static
void
ShowUsage()
{
    printf("Usage: lw-ypcat {--level [0]} {--batchsize [1..1000]} {-k} <map name>\n");
    printf("\n");
    printf("-k : print keys.\n");
}

static
VOID
PrintMapInfo_0(
    PLSA_NSS_ARTEFACT_INFO_0 pMapInfo,
    BOOLEAN bPrintKeys
    )
{
    if (!bPrintKeys)
    {
        fprintf(stdout, "%s\n",
                        (IsNullOrEmptyString(pMapInfo->pszName) ? "" : pMapInfo->pszName));
    }
    else
    {
        fprintf(stdout, "%s %s\n",
                (IsNullOrEmptyString(pMapInfo->pszName) ? "" : pMapInfo->pszName),
                (IsNullOrEmptyString(pMapInfo->pszValue) ? "" : pMapInfo->pszValue));
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

