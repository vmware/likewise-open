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

static PCSTR LOGIN_CONFIG_PATH = "/etc/security/login.cfg";

static
 BOOLEAN IsComment(PSTR pszLine)
{
	PSTR pszTmp = pszLine;

	if (IsNullOrEmptyString(pszLine))
		return TRUE;

	while (*pszTmp != '\0' && isspace(*pszTmp))
		pszTmp++;

	return *pszTmp == '*' || *pszTmp == '\0';
}

CENTERROR DJFixLoginConfigFile(PCSTR pszPath)
{
	CENTERROR ceError = CENTERROR_SUCCESS;
	PCSTR pszFilePath = NULL;
	CHAR szBuf[1024 + 1];
	PSTR pszTmpPath = NULL;
	PCSTR pszRegExp = "^[[:space:]]*auth_type[[:space:]]*=.*$";
	BOOLEAN bPatternExists = FALSE;
	BOOLEAN bFileExists = FALSE;
	BOOLEAN bRemoveFile = FALSE;
	FILE *fp = NULL;
	FILE *fp_orig = NULL;
	FILE *fp_new = NULL;

	if (IsNullOrEmptyString(pszPath))
		pszFilePath = LOGIN_CONFIG_PATH;
	else
		pszFilePath = pszPath;

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

		fprintf(fp, "\tauth_type = PAM_AUTH\n");
		fclose(fp);
		fp = NULL;
	} else {
		ceError =
		    CTAllocateMemory(strlen(pszFilePath) +
				     sizeof(".domainjoin"),
				     (PVOID *) & pszTmpPath);
		BAIL_ON_CENTERIS_ERROR(ceError);

		strcpy(pszTmpPath, pszFilePath);
		strcat(pszTmpPath, ".domainjoin");

		if ((fp_orig = fopen(pszFilePath, "r")) == NULL) {
			ceError = CTMapSystemError(errno);
			BAIL_ON_CENTERIS_ERROR(ceError);
		}

		if ((fp_new = fopen(pszTmpPath, "w")) == NULL) {
			ceError = CTMapSystemError(errno);
			BAIL_ON_CENTERIS_ERROR(ceError);
		}

		bRemoveFile = TRUE;

		while (1) {
			if (fgets(szBuf, 1024, fp_orig) == NULL) {
				if (feof(fp_orig))
					break;
				else {
					ceError = CTMapSystemError(errno);
					BAIL_ON_CENTERIS_ERROR(ceError);
				}
			}

			if (strstr(szBuf, "auth_type") && !IsComment(szBuf)) {
				fputs("\tauth_type = PAM_AUTH\n", fp_new);
			} else {
				fputs(szBuf, fp_new);
			}
		}

		fclose(fp_new);
		fp_new = NULL;
		fclose(fp_orig);
		fp_orig = NULL;

	}

	ceError = CTBackupFile(pszFilePath);
	BAIL_ON_CENTERIS_ERROR(ceError);

	ceError = CTMoveFile(pszTmpPath, pszFilePath);
	BAIL_ON_CENTERIS_ERROR(ceError);

	bRemoveFile = FALSE;

      done:
      error:
	if (fp)
		fclose(fp);

	if (fp_orig)
		fclose(fp_orig);

	if (fp_new)
		fclose(fp_new);

	if (bRemoveFile)
		CTRemoveFile(pszTmpPath);

	if (pszTmpPath)
		CTFreeString(pszTmpPath);

	return ceError;
}
#endif
