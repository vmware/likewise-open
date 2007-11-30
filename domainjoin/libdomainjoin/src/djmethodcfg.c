/*
 * Copyright (C) Centeris Corporation 2004-2007
 * Copyright (C) Likewise Software 2007.  
 * All rights reserved.
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "domainjoin.h"

#if defined(_AIX)
CENTERROR DJFixMethodsConfigFile()
{
	CENTERROR ceError = CENTERROR_SUCCESS;
	PCSTR pszFilePath = "/usr/lib/security/methods.cfg";
	PSTR pszTmpPath = NULL;
	PCSTR pszRegExp =
	    "^[[:space:]]*program[[:space:]]*=[[:space:]]*\\/usr\\/lib\\/security\\/LWIDENTITY[[:space:]]*$";
	BOOLEAN bPatternExists = FALSE;
	BOOLEAN bFileExists = FALSE;
	BOOLEAN bRemoveFile = FALSE;
	FILE *fp = NULL;

	ceError = CTCheckFileExists(pszFilePath, &bFileExists);
	BAIL_ON_CENTERIS_ERROR(ceError);

	if (!bFileExists)
		goto done;

	ceError =
	    CTCheckFileHoldsPattern(pszFilePath, pszRegExp, &bPatternExists);
	BAIL_ON_CENTERIS_ERROR(ceError);

	if (!bPatternExists) {
		ceError =
		    CTAllocateMemory(strlen(pszFilePath) +
				     sizeof(".domainjoin"),
				     (PVOID *) & pszTmpPath);
		BAIL_ON_CENTERIS_ERROR(ceError);

		strcpy(pszTmpPath, pszFilePath);
		strcat(pszTmpPath, ".domainjoin");

		ceError = CTCopyFileWithOriginalPerms(pszFilePath, pszTmpPath);
		BAIL_ON_CENTERIS_ERROR(ceError);

		bRemoveFile = TRUE;

		if ((fp = fopen(pszTmpPath, "a")) == NULL) {
			ceError = CTMapSystemError(errno);
			BAIL_ON_CENTERIS_ERROR(ceError);
		}

		fprintf(fp, "\nLWIDENTITY:\n");
		fprintf(fp, "\tprogram = /usr/lib/security/LWIDENTITY\n");
		fclose(fp);
		fp = NULL;

		ceError = CTBackupFile(pszFilePath);
		BAIL_ON_CENTERIS_ERROR(ceError);

		ceError = CTMoveFile(pszTmpPath, pszFilePath);
		BAIL_ON_CENTERIS_ERROR(ceError);

		bRemoveFile = FALSE;
	}

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
#endif
