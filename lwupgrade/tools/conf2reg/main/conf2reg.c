/*
 * Copyright (c) Likewise Software.  All rights reserved.
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
 * license@likewise.com
 */

/*
 * Copyright (C) Likewise Software. All rights reserved.
 *
 * Module Name:
 *
 * Abstract:
 *
 * Authors:
 * 
 */
#include "includes.h"

extern
DWORD
EventlogConfFileToRegFile(
    PCSTR pszConfFile,
    PCSTR pszRegFile
    );

extern
DWORD
LsassConfFileToRegFile(
    PCSTR pszConfFile,
    PCSTR pszRegFile
    );

extern
DWORD
NetlogonConfFileToRegFile(
    PCSTR pszConfFile,
    PCSTR pszRegFile
    );

extern
DWORD
LwiauthConfFileToRegFile(
    PCSTR pszConfFile,
    PCSTR pszSecretsFile,
    PCSTR pszRegFile
    );

extern
DWORD
LwioConfFileToRegFile(
    PCSTR pszConfFile,
    PCSTR pszRegFile
    );

extern
DWORD
SqliteMachineAccountToPstore(
    PCSTR pszSqlDb
    );

extern
DWORD
TestParseConfFileToRegFile(
    PCSTR pszConfFile,
    PCSTR pszRegFile
    );

extern
DWORD
TestSambaParseConfFileToRegFile(
    PCSTR pszConfFile,
    PCSTR pszRegFile
    );


static
VOID
PrintUsage(
    PCSTR pszAdditionalMessage
    );

int
main(
    int argc,
    char *argv[]
    )
{
    DWORD dwError = 0;

    if ( argc < 2 )
    {
        exit(1);
    }

    if (!strcmp(argv[1], "--eventlog"))
    {
        if (argc < 4)
        {
            PrintUsage("--eventlog requires two arguments.");
        }
        else
        {
            dwError = EventlogConfFileToRegFile(argv[2], argv[3]);
        }
        goto cleanup;
    }
    if (!strcmp(argv[1], "--lsass"))
    {
        if (argc < 4)
        {
            PrintUsage("--lsass requires two arguments.");
        }
        else
        {
            dwError = LsassConfFileToRegFile(argv[2], argv[3]);
        }
        goto cleanup;
    }
    if (!strcmp(argv[1], "--lwio"))
    {
        if (argc < 4)
        {
            PrintUsage("--lwio requires two arguments.");
        }
        else
        {
            dwError = LwioConfFileToRegFile(argv[2], argv[3]);
        }
        goto cleanup;
    }
    if (!strcmp(argv[1], "--netlogon"))
    {
        if (argc < 4)
        {
            PrintUsage("--netlogon requires two arguments.");
        }
        else
        {
            dwError = NetlogonConfFileToRegFile(argv[2], argv[3]);
        }
        goto cleanup;
    }
    if (!strcmp(argv[1], "--lwiauth"))
    {
        if (argc < 5)
        {
            PrintUsage("--lwiauth requires three arguments.");
        }
        else
        {
            dwError = LwiauthConfFileToRegFile(argv[2], argv[3], argv[4]);
        }
        goto cleanup;
    }
    if (!strcmp(argv[1], "--pstore-sqlite"))
    {
        if (argc < 3)
        {
            PrintUsage("--pstore-sqlite requires one argument.");
        }
        else
        {
            dwError = SqliteMachineAccountToPstore(argv[2]);
        }
        goto cleanup;
    }
    PrintUsage(NULL);

cleanup:

    if (dwError)
    {
        fprintf(stderr, "Error %d\n", dwError);
    }
    return 0;
}

static
VOID
PrintUsage(
    PCSTR pszAdditionalMessage
    )
{
    fputs(
"conf2reg: Convert Likewise daemon configuration file into\n"
"          registry import file. Also, import old databases.    \n"
"\n"
"--lsass CONF REG\n"
"  Convert lsass 5.x configuration file to registry\n"
"  import file.\n"
"\n"
"--lwio CONF REG\n"
"  Convert lwio 5.x configuration file to registry import\n"
"  file.\n"
"\n"
"--netlogon CONF REG\n"
"  Convert netlogon 5.x configuration file to registry\n"
"  import file.\n"
"\n"
"--eventlog CONF REG\n"
"  Convert eventlog 5.x configuration file to registry\n"
"  import file.\n"
"\n"
"--lwiauth CONF TDB REG\n"
"  Import 4.1 machine account into pstore using\n"
"  lwiauthd.conf and secrets.tdb file. Also generates\n"
"  registry file with some settings for import.\n"
"\n"
"--pstore-sqlite SQLDB\n"
"  Import machine account stored in sqlite database\n"
"  (as used in versions 5.0 through 5.3) into pstore.\n"

, stderr);

   if (pszAdditionalMessage)
   {
       fputs("\n\n", stderr);
       fputs(pszAdditionalMessage, stderr);
   }
}
