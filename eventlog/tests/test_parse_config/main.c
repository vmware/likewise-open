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
 *        Likewise Eventlog
 *
 *        Test Program for exercising EVTParseConfigFile
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *
 */

#define _POSIX_PTHREAD_SEMANTICS 1

#include "config.h"
#include "eventsys.h"
#include "eventlog.h"
#include "eventdefs.h"
#include "eventutils.h"

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
    PBOOLEAN pbSkipSection,
    PBOOLEAN pbContinue
    )
{

    printf("<SECTION Name=\"%s\">\n", pszSectionName);

    *pbSkipSection = FALSE;
    *pbContinue = TRUE;

    return 0;
}

DWORD
ConfigComment(
    PCSTR    pszComment,
    PBOOLEAN pbContinue
    )
{
    printf("<!-- %s -->\n",
        (IsNullOrEmptyString(pszComment) ? "" : pszComment));

    *pbContinue = TRUE;

    return 0;
}


DWORD
ConfigNameValuePair(
    PCSTR    pszName,
    PCSTR    pszValue,
    PBOOLEAN pbContinue
    )
{
    printf("\t<NAME Id=\"%s\" Value=\"%s\"/>\n",
        (IsNullOrEmptyString(pszName) ? "" : pszName),
        (IsNullOrEmptyString(pszValue) ? "" : pszValue));

    *pbContinue = TRUE;

    return 0;
}

DWORD
ConfigEndSection(
    PCSTR pszSectionName,
    PBOOLEAN pbContinue
    )
{
    printf("</SECTION>\n");

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

    PSTR  pszConfigFilePath;

    EVTInitLoggingToFile(LOG_LEVEL_VERBOSE, "");

    dwError = ParseArgs(argc, argv,
        &pszConfigFilePath);
    BAIL_ON_EVT_ERROR(dwError);

    printf("<CONFIG>\n\n");

    dwError = EVTParseConfigFile(
        pszConfigFilePath,
        &ConfigStartSection,
        &ConfigComment,
        &ConfigNameValuePair,
        &ConfigEndSection);
    BAIL_ON_EVT_ERROR(dwError);

    printf("\n</CONFIG>\n");

cleanup:

    EVT_SAFE_FREE_STRING(pszConfigFilePath);

    EVTCloseLog();

    return (dwError);

error:

    fprintf(stderr, "Failed to parse config file. Error code [%ld]\n", (unsigned long)dwError);

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
                    dwError = EVTAllocateString(pszArg, &pszConfigFilePath);
                    BAIL_ON_EVT_ERROR(dwError);
                    parseMode = PARSE_MODE_DONE;
                }
                break;

            case PARSE_MODE_DONE:
                ShowUsage();
                break;


        }

    } while (iArg < argc);

    if (IsNullOrEmptyString(pszConfigFilePath))
    {
        ShowUsage();
        exit(1);
    }

    *ppszConfigFilePath = pszConfigFilePath;

cleanup:

    return dwError;

error:

    EVT_SAFE_FREE_STRING(pszConfigFilePath);

    *ppszConfigFilePath = NULL;

    goto cleanup;
}

void
ShowUsage()
{
    printf("Usage: test-parse-config-file <configFilePath>\n");
}


