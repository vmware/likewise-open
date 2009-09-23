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
 *        regexport.c
 *
 * Abstract:
 *
 *        Registry
 *
 *        Registry Export Utility
 *
 * Authors: Wei Fu (wfu@likewise.com)
 */
#include <parse/includes.h>
#include <regclient.h>
#include <reg/reg.h>
#include <shellutil/rsutils.h>


int main(int argc, char *argv[])
{
    DWORD dwError;
    HANDLE hReg = NULL;
    char szRegFileName[PATH_MAX + 1];
    FILE* fp = NULL;

    if (argc == 1)
    {
        printf("usage: %s regfile.reg\n", argv[0]);
        return 0;
    }

    strcpy(szRegFileName, argv[1]);

    LwStripWhitespace(szRegFileName, TRUE, TRUE);

    fp = fopen(szRegFileName, "w");
    if (fp == NULL) {
        dwError = errno;
        goto error;
    }

    dwError = RegOpenServer(&hReg);
    BAIL_ON_REG_ERROR(dwError);

    dwError = RegShellUtilExport(hReg,
                                 fp,
                                 NULL,
                                 NULL,
                                 0);
    BAIL_ON_REG_ERROR(dwError);

finish:
    if (fp)
    {
        fclose(fp);
    }
    RegCloseServer(hReg);
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
