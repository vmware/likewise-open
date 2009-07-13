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
 *        Test Program for exercising LsaParseConfigFile
 *
 * Authors: Brian Dunstan (bdunstan@likewisesoftware.com)
 *
 */

#define _POSIX_PTHREAD_SEMANTICS 1

#include "config.h"
#include "lsasystem.h"
#include "lsadef.h"
#include "lsa/lsa.h"
#include "lwmem.h"
#include "lwstr.h"
#include "lwsecurityidentifier.h"
#include "lsautils.h"

DWORD
ParseArgs(
    int    argc,
    char*  argv[],
    PSTR*  ppszConfigFilePath
    );

VOID
ShowUsage();


DWORD
ConfigStartSection(
    PCSTR    pszSectionName,
    PVOID    pData,
    PBOOLEAN pbSkipSection,
    PBOOLEAN pbContinue
    )
{
    PDWORD pdwCallbackCount = (PDWORD)pData;
    
    printf("CALLBACK #%u: <SECTION Name=\"%s\">\n", 
        *pdwCallbackCount,
        pszSectionName);
    
    (*pdwCallbackCount)++;
    
    *pbSkipSection = FALSE;
    *pbContinue = TRUE;
    
    return 0;
}
                        
DWORD
ConfigComment(
    PCSTR    pszComment,
    PVOID    pData,
    PBOOLEAN pbContinue
    )
{
    PDWORD pdwCallbackCount = (PDWORD)pData;
    
    printf("CALLBACK #%u: <!-- %s -->\n", 
        *pdwCallbackCount,
        (IsNullOrEmptyString(pszComment) ? "" : pszComment));
    
    (*pdwCallbackCount)++;
    
    *pbContinue = TRUE;
    
    return 0;
}
    

DWORD
ConfigNameValuePair(
    PCSTR    pszName,
    PCSTR    pszValue,
    PVOID    pData,
    PBOOLEAN pbContinue
    )
{
    PDWORD pdwCallbackCount = (PDWORD)pData;
    
    printf("CALLBACK #%u: \t<NAME Id=\"%s\" Value=\"%s\"/>\n", 
        *pdwCallbackCount,
        (IsNullOrEmptyString(pszName) ? "" : pszName), 
        (IsNullOrEmptyString(pszValue) ? "" : pszValue));
    
    (*pdwCallbackCount)++;
    
    *pbContinue = TRUE;
    
    return 0;
}

DWORD
ConfigEndSection(
    PCSTR pszSectionName,
    PVOID    pData,
    PBOOLEAN pbContinue
    )
{
    PDWORD pdwCallbackCount = (PDWORD)pData;
    
    printf("CALLBACK #%u: </SECTION>\n", *pdwCallbackCount);
    
    (*pdwCallbackCount)++;
    
    *pbContinue = TRUE;
    
    return 0;
}


int
main(
    int argc,
    char* argv[]
    )
{
    DWORD dwError = 0;
    
    PSTR  pszConfigFilePath = NULL;
    
    DWORD dwCallbackCount = 0;
    
    dwError = ParseArgs(argc, argv, 
        &pszConfigFilePath);
    BAIL_ON_LSA_ERROR(dwError);
    
    
    printf("<CONFIG>\n\n");
    
    dwError = LsaParseConfigFile(
        pszConfigFilePath,
        LSA_CFG_OPTION_STRIP_ALL,
        &ConfigStartSection,
        &ConfigComment,
        &ConfigNameValuePair,
        &ConfigEndSection,
        &dwCallbackCount);
    BAIL_ON_LSA_ERROR(dwError);

    printf("\n</CONFIG>\n");
    
    printf("\n\nTotal callbacks: %u\n", dwCallbackCount);
    
cleanup:

    LSA_SAFE_FREE_STRING(pszConfigFilePath);
    
    return (dwError);

error:

    fprintf(stderr, "Failed to parse config file. Error code [%u]\n", dwError);

    goto cleanup;
}

DWORD
ParseArgs(
    int    argc,
    char*  argv[],
    PSTR*  ppszConfigFilePath
    )
{
    typedef enum {
            PARSE_MODE_OPEN = 0,
            PARSE_MODE_DONE
        } ParseMode;
        
    DWORD dwError = 0;
    int iArg = 1;
    PSTR  pszConfigFilePath = NULL;
    PSTR  pszArg = NULL;
    ParseMode parseMode = PARSE_MODE_OPEN;

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
                else
                {
                    dwError = LsaAllocateString(pszArg, &pszConfigFilePath);
                    BAIL_ON_LSA_ERROR(dwError);
                    parseMode = PARSE_MODE_DONE;
                }
                break;

            default:

                dwError = LW_ERROR_INTERNAL;
                BAIL_ON_LSA_ERROR(dwError);
        }

    } while ((parseMode != PARSE_MODE_DONE) && (iArg < argc));

    if (IsNullOrEmptyString(pszConfigFilePath))
    {
       ShowUsage();
       exit(1);
    }

    *ppszConfigFilePath = pszConfigFilePath;

cleanup:
    
    return dwError;

error:

    LSA_SAFE_FREE_STRING(pszConfigFilePath);

    *ppszConfigFilePath = NULL;

    goto cleanup;
}

void
ShowUsage()
{
    printf("Usage: test-parse-config-file <configFilePath>\n");
}

