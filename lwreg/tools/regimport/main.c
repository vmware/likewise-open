/*
 * Copyright Likewise Software    2004-2009
 * All rights reserved.
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the license, or (at
 * your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser
 * General Public License for more details.  You should have received a copy
 * of the GNU Lesser General Public License along with this program.  If
 * not, see <http://www.gnu.org/licenses/>.
 *
 * LIKEWISE SOFTWARE MAKES THIS SOFTWARE AVAILABLE UNDER OTHER LICENSING
 * TERMS AS WELL.  IF YOU HAVE ENTERED INTO A SEPARATE LICENSE AGREEMENT
 * WITH LIKEWISE SOFTWARE, THEN YOU MAY ELECT TO USE THE SOFTWARE UNDER THE
 * TERMS OF THAT SOFTWARE LICENSE AGREEMENT INSTEAD OF THE TERMS OF THE GNU
 * LESSER GENERAL PUBLIC LICENSE, NOTWITHSTANDING THE ABOVE NOTICE.  IF YOU
 * HAVE QUESTIONS, OR WISH TO REQUEST A COPY OF THE ALTERNATE LICENSING
 * TERMS OFFERED BY LIKEWISE SOFTWARE, PLEASE CONTACT LIKEWISE SOFTWARE AT
 * license@likewisesoftware.com
 */

/*
 * Copyright (C) Likewise Software. All rights reserved.
 *
 * Module Name:
 *
 *        lw_regimport.c
 *
 * Abstract:
 *
 *        Registry
 *
 *        Registry import tool (import a win32 compatible registry file)
 *
 * Authors: Wei Fu (wfu@likewise.com)
 *          Adam Bernstein (abernstein@likewise.com)
 *
 */
#include <parse/includes.h>
#include <regclient.h>
#include <reg/reg.h>
#include <shellutil/rsutils.h>


int main(int argc, char *argv[])
{
    DWORD dwError;
    HANDLE parseH = NULL;
    IMPORT_CONTEXT ctx = {0};

    if (argc == 1)
    {
        printf("usage: %s regfile.reg\n", argv[0]);
        return 0;
    }

    dwError = RegOpenServer(&ctx.hReg);
    BAIL_ON_REG_ERROR(dwError);

    dwError = RegParseOpen(argv[1], RegShellUtilImportCallback, &ctx, &parseH);
    if (dwError)
    {
        fprintf(stderr, "RegParseOpen: failed %d\n", dwError);
        return 1;
    }

    dwError = RegParseRegistry(parseH);

finish:
    RegCloseServer(ctx.hReg);
    RegParseClose(parseH);
    fflush(stdout);
    return dwError;
error:
    if (dwError)
    {
        PrintError(NULL, dwError);
    }

    dwError = dwError ? 1 : 0;

    goto finish;
}
