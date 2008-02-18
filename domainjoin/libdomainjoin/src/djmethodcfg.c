/*
 * Copyright (C) Centeris Corporation 2004-2007
 * Copyright (C) Likewise Software    2007-2008
 * All rights reserved.
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as 
 * published by the Free Software Foundation; either version 2.1 of 
 * the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public 
 * License along with this program.  If not, see 
 * <http://www.gnu.org/licenses/>.
 */

/* ex: set tabstop=4 expandtab shiftwidth=4: */
#include "domainjoin.h"

static PCSTR methodsPath = "/usr/lib/security/methods.cfg";

CENTERROR
DJHasMethodsCfg(BOOLEAN *exists)
{
    return CTCheckFileExists(methodsPath, exists);
}

CENTERROR
DJIsMethodsCfgConfigured(BOOLEAN *configured)
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    PCSTR pszRegExp = "^[[:space:]]*program[[:space:]]*=[[:space:]]*\\/usr\\/lib\\/security\\/LWIDENTITY[[:space:]]*$";
    BOOLEAN bPatternExists = FALSE;
    BOOLEAN bFileExists = FALSE;

    *configured = FALSE;

    ceError = CTCheckFileExists(methodsPath, &bFileExists);
    GOTO_CLEANUP_ON_CENTERROR(ceError);

    if (!bFileExists)
    {
        *configured = TRUE;
        goto cleanup;
    }

    ceError = CTCheckFileHoldsPattern(methodsPath, pszRegExp, &bPatternExists);
    GOTO_CLEANUP_ON_CENTERROR(ceError);

    if(bPatternExists)
        *configured = TRUE;

cleanup:
    return ceError;
}

CENTERROR
DJUnconfigMethodsConfigFile()
{
    BOOLEAN exists;
    CENTERROR ceError = CENTERROR_SUCCESS;

    ceError = DJHasMethodsCfg(&exists);
    BAIL_ON_CENTERIS_ERROR(ceError);
    if(!exists)
        goto error;

    ceError = CTRunSedOnFile(methodsPath, methodsPath, FALSE,
            "/^[ \t]*[^ \t#*].*LWIDENTITY.*/d");
    BAIL_ON_CENTERIS_ERROR(ceError);

error:
    return ceError;
}

CENTERROR
DJFixMethodsConfigFile()
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    PSTR pszTmpPath = NULL;
    BOOLEAN bRemoveFile = FALSE;
    BOOLEAN isConfigured = FALSE;
    FILE* fp = NULL;

    ceError = DJIsMethodsCfgConfigured(&isConfigured);
    BAIL_ON_CENTERIS_ERROR(ceError);
    if(isConfigured)
        goto done;

    ceError = CTAllocateMemory(strlen(methodsPath)+sizeof(".domainjoin"),
                               (PVOID*)&pszTmpPath);
    BAIL_ON_CENTERIS_ERROR(ceError);

    strcpy(pszTmpPath, methodsPath);
    strcat(pszTmpPath, ".domainjoin");

    ceError = CTCopyFileWithOriginalPerms(methodsPath, pszTmpPath);
    BAIL_ON_CENTERIS_ERROR(ceError);

    bRemoveFile = TRUE;

    if ((fp = fopen(pszTmpPath, "a")) == NULL) {
        ceError = CTMapSystemError(errno);
        BAIL_ON_CENTERIS_ERROR(ceError);
    }

    fprintf(fp, "\nLWIDENTITY:\n");
    fprintf(fp, "\tprogram = /usr/lib/security/LWIDENTITY\n");
    fclose(fp); fp = NULL;

    ceError = CTBackupFile(methodsPath);
    BAIL_ON_CENTERIS_ERROR(ceError);

    ceError = CTMoveFile(pszTmpPath, methodsPath);
    BAIL_ON_CENTERIS_ERROR(ceError);

    bRemoveFile = FALSE;

done:
error:

    if (fp)
        fclose(fp);

    if (bRemoveFile)
        CTRemoveFile(pszTmpPath);

    if (pszTmpPath)
        CTFreeString(pszTmpPath);

    return ceError;
}
