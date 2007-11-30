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

#define NSSWITCH_CONF_PATH "/etc/nsswitch.conf"

static
 CENTERROR GetTmpPath(PCSTR pszOriginalPath, PSTR * ppszTmpPath)
{
	CENTERROR ceError = CENTERROR_SUCCESS;
	PCSTR pszSuffix = ".domainjoin";
	PSTR pszTmpPath = NULL;

	ceError =
	    CTAllocateMemory(strlen(pszOriginalPath) + strlen(pszSuffix) + 1,
			     (PVOID *) & pszTmpPath);
	BAIL_ON_CENTERIS_ERROR(ceError);

	strcpy(pszTmpPath, pszOriginalPath);
	strcat(pszTmpPath, pszSuffix);

	*ppszTmpPath = pszTmpPath;

	return ceError;

      error:

	if (pszTmpPath)
		CTFreeString(pszTmpPath);

	return ceError;
}

CENTERROR UnConfigureNameServiceSwitch()
{
	CENTERROR ceError = CENTERROR_SUCCESS;
	BOOLEAN bFileExists = FALSE;
	PCSTR pszFilePath = NSSWITCH_CONF_PATH;
	PCSTR pszPasswdExp = "^[[:space:]]*passwd[[:space:]]*:.*lwidentity.*$";
	PCSTR pszGroupExp = "^[[:space:]]*groups[[:space:]]*:.*lwidentity.*$";
	PSTR pszTmpPath = NULL;
	regex_t passwd_rx;
	regex_t group_rx;
	FILE *fpSrc = NULL;
	FILE *fpDst = NULL;
	CHAR szBuf[1024 + 1];
	CHAR szTmp[1024 + 1];
	BOOLEAN bRemoveFile = FALSE;
	PSTR pszToken = NULL;
	PSTR pszMarker = NULL;
	BOOLEAN bIsPasswdEntry = TRUE;
	BOOLEAN bWriteOrig = FALSE;
	DWORD iToken = 0;

	memset(&passwd_rx, 0, sizeof(regex_t));
	memset(&group_rx, 0, sizeof(regex_t));

	ceError = CTCheckFileExists(pszFilePath, &bFileExists);
	BAIL_ON_CENTERIS_ERROR(ceError);

	if (bFileExists) {

		if (regcomp(&passwd_rx, pszPasswdExp, REG_EXTENDED) < 0) {
			ceError = CENTERROR_REGEX_COMPILE_FAILED;
			BAIL_ON_CENTERIS_ERROR(ceError);
		}

		if (regcomp(&group_rx, pszGroupExp, REG_EXTENDED) < 0) {
			ceError = CENTERROR_REGEX_COMPILE_FAILED;
			BAIL_ON_CENTERIS_ERROR(ceError);
		}

		ceError = GetTmpPath(pszFilePath, &pszTmpPath);
		BAIL_ON_CENTERIS_ERROR(ceError);

		if ((fpSrc = fopen(pszFilePath, "r")) == NULL) {
			ceError = CTMapSystemError(errno);
			BAIL_ON_CENTERIS_ERROR(ceError);
		}

		if ((fpDst = fopen(pszTmpPath, "w")) == NULL) {
			ceError = CTMapSystemError(errno);
			BAIL_ON_CENTERIS_ERROR(ceError);
		}

		bRemoveFile = TRUE;

		while (1) {

			bWriteOrig = FALSE;

			if (fgets(szBuf, 1024, fpSrc) == NULL) {

				if (feof(fpSrc)) {
					break;
				} else {
					ceError = CTMapSystemError(errno);
					BAIL_ON_CENTERIS_ERROR(ceError);
				}
			}

			memcpy(szTmp, szBuf, sizeof(szBuf));

			CTStripWhitespace(szTmp);

			if (*szTmp == '#') {

				/* Comment --- write out the original string */
				bWriteOrig = TRUE;

			} else
			    if (!regexec(&passwd_rx, szTmp, (size_t) 0, NULL, 0)
				|| !regexec(&group_rx, szTmp, (size_t) 0, NULL,
					    0)) {

				bIsPasswdEntry =
				    !regexec(&passwd_rx, szTmp, (size_t) 0,
					     NULL, 0);

				if (bIsPasswdEntry) {
					strcpy(szBuf, "passwd: ");
				} else {
					strcpy(szBuf, "group: ");
				}

				pszToken = strchr(szTmp, ':');
				if (pszToken == NULL) {
					ceError =
					    CENTERROR_DOMAINJOIN_NSSWITCH_EDIT_FAIL;
					BAIL_ON_CENTERIS_ERROR(ceError);
				}

				pszMarker = pszToken + 1;

				pszToken = strtok(pszMarker, " \t");
				CTStripWhitespace(pszToken);
				iToken = 0;
				while (!IsNullOrEmptyString(pszToken)) {

					if (strcmp(pszToken, "lwidentity")) {

						if (iToken) {
							strcat(szBuf, " ");
						}
						strcat(szBuf, pszToken);

					}

					iToken++;

					pszToken = strtok(NULL, " \t");
				}

				// If lwidentity was the only entry
				// and we removed that now, don't write
				// an empty entry into the file
				if (iToken > 1) {
					fprintf(fpDst, "%s\n", szBuf);
				} else {
					if (bIsPasswdEntry) {
						fprintf(fpDst,
							"passwd: files\n");
					} else {
						fprintf(fpDst,
							"group: files\n");
					}
				}

			} else {

				bWriteOrig = TRUE;

			}

			if (bWriteOrig) {
				if (fputs(szBuf, fpDst) == EOF) {
					ceError = CTMapSystemError(errno);
					BAIL_ON_CENTERIS_ERROR(ceError);
				}
			}

		}

		if (fpSrc) {
			fclose(fpSrc);
			fpSrc = NULL;
		}

		if (fpDst) {
			fclose(fpDst);
			fpDst = NULL;
		}

		ceError = CTBackupFile(pszFilePath);
		BAIL_ON_CENTERIS_ERROR(ceError);

		ceError = CTMoveFile(pszTmpPath, pszFilePath);
		BAIL_ON_CENTERIS_ERROR(ceError);

		bRemoveFile = FALSE;
	}

      error:

	regfree(&passwd_rx);

	regfree(&group_rx);

	if (fpSrc)
		fclose(fpSrc);

	if (fpDst)
		fclose(fpDst);

	if (bRemoveFile)
		CTRemoveFile(pszTmpPath);

	if (pszTmpPath)
		CTFreeString(pszTmpPath);

	return ceError;
}

CENTERROR ConfigureNameServiceSwitch()
{
	CENTERROR ceError = CENTERROR_SUCCESS;
	BOOLEAN bFileExists = FALSE;
	PCSTR pszFilePath = NSSWITCH_CONF_PATH;
	PCSTR pszPasswdExp = "^[[:space:]]*passwd[[:space:]]*:.*$";
	PCSTR pszGroupExp = "^[[:space:]]*group[[:space:]]*:.*$";
	PSTR pszTmpPath = NULL;
	regex_t passwd_rx;
	regex_t group_rx;
	FILE *fpSrc = NULL;
	FILE *fpDst = NULL;
	CHAR szBuf[1024 + 1];
	CHAR szTmp[1024 + 1];
	BOOLEAN bRemoveFile = FALSE;
	PSTR pszToken = NULL;
	PSTR pszMarker = NULL;
	BOOLEAN bAddTag = FALSE;
	BOOLEAN bIsPasswdEntry = TRUE;
	BOOLEAN bPasswdTagAdded = FALSE;
	BOOLEAN bGroupsTagAdded = FALSE;
	BOOLEAN bWriteOrig = FALSE;

	memset(&passwd_rx, 0, sizeof(regex_t));
	memset(&group_rx, 0, sizeof(regex_t));

	ceError = CTCheckFileExists(pszFilePath, &bFileExists);
	BAIL_ON_CENTERIS_ERROR(ceError);

#ifdef _HPUX_SOURCE

#define NSSWITCH_LWIDEFAULTS "/etc/nsswitch.lwi_defaults"

	/* HP-UX 11.xx does not appear to have an nsswitch file in
	   place by default. If we don't find on already installed,
	   use our own */

	if (!bFileExists) {
		pszFilePath = NSSWITCH_LWIDEFAULTS;
		ceError = CTCheckFileExists(pszFilePath, &bFileExists);
		BAIL_ON_CENTERIS_ERROR(ceError);

		if (bFileExists) {
			CTCopyFileWithOriginalPerms(pszFilePath,
						    NSSWITCH_CONF_PATH);
			BAIL_ON_CENTERIS_ERROR(ceError);
			pszFilePath = NSSWITCH_CONF_PATH;
		}
	}
#endif				/* _HPUX_SOURCE */

	if (bFileExists) {

		if (regcomp(&passwd_rx, pszPasswdExp, REG_EXTENDED) < 0) {
			ceError = CENTERROR_REGEX_COMPILE_FAILED;
			BAIL_ON_CENTERIS_ERROR(ceError);
		}

		if (regcomp(&group_rx, pszGroupExp, REG_EXTENDED) < 0) {
			ceError = CENTERROR_REGEX_COMPILE_FAILED;
			BAIL_ON_CENTERIS_ERROR(ceError);
		}

		ceError = GetTmpPath(pszFilePath, &pszTmpPath);
		BAIL_ON_CENTERIS_ERROR(ceError);

		if ((fpSrc = fopen(pszFilePath, "r")) == NULL) {
			ceError = CTMapSystemError(errno);
			BAIL_ON_CENTERIS_ERROR(ceError);
		}

		if ((fpDst = fopen(pszTmpPath, "w")) == NULL) {
			ceError = CTMapSystemError(errno);
			BAIL_ON_CENTERIS_ERROR(ceError);
		}

		bRemoveFile = TRUE;

		while (1) {

			bWriteOrig = FALSE;
			bAddTag = FALSE;

			if (fgets(szBuf, 1024, fpSrc) == NULL) {

				if (feof(fpSrc)) {
					break;
				} else {
					ceError = CTMapSystemError(errno);
					BAIL_ON_CENTERIS_ERROR(ceError);
				}
			}

			memcpy(szTmp, szBuf, sizeof(szBuf));

			CTStripWhitespace(szTmp);

			if (*szTmp == '#') {

				/* Comment --- write out the original string */
				bWriteOrig = TRUE;

			} else
			    if ((bIsPasswdEntry =
				 !regexec(&passwd_rx, szTmp, (size_t) 0, NULL,
					  0))
				|| !regexec(&group_rx, szTmp, (size_t) 0, NULL,
					    0)) {

				pszToken = strchr(szTmp, ':');
				if (pszToken == NULL) {
					ceError =
					    CENTERROR_DOMAINJOIN_NSSWITCH_EDIT_FAIL;
					BAIL_ON_CENTERIS_ERROR(ceError);
				}

				pszMarker = pszToken + 1;

				bAddTag = TRUE;

				pszToken = strtok(pszMarker, " \t");
				CTStripWhitespace(pszToken);

				while (!IsNullOrEmptyString(pszToken)) {

					if (!strcmp(pszToken, "lwidentity")) {
						bAddTag = FALSE;
						if (bIsPasswdEntry)
							bPasswdTagAdded = TRUE;
						else
							bGroupsTagAdded = TRUE;
						break;
					}

					pszToken = strtok(NULL, " \t");

				}

				if (bAddTag) {

					CTStripTrailingWhitespace(szBuf);
					fprintf(fpDst, "%s lwidentity\n",
						szBuf);
					if (bIsPasswdEntry)
						bPasswdTagAdded = TRUE;
					else
						bGroupsTagAdded = TRUE;

				} else {

					bWriteOrig = TRUE;

				}
			} else {

				bWriteOrig = TRUE;
			}

			if (bWriteOrig) {
				if (fputs(szBuf, fpDst) == EOF) {
					ceError = CTMapSystemError(errno);
					BAIL_ON_CENTERIS_ERROR(ceError);
				}
			}

		}

		if (!bPasswdTagAdded) {
			if (fprintf(fpDst, "passwd: files lwidentity\n") < 0) {
				ceError = CTMapSystemError(errno);
				BAIL_ON_CENTERIS_ERROR(ceError);
			}
		}

		if (!bGroupsTagAdded) {
			if (fprintf(fpDst, "group: files lwidentity\n") < 0) {
				ceError = CTMapSystemError(errno);
				BAIL_ON_CENTERIS_ERROR(ceError);
			}
		}

		if (fpSrc) {
			fclose(fpSrc);
			fpSrc = NULL;
		}

		if (fpDst) {
			fclose(fpDst);
			fpDst = NULL;
		}

		ceError = CTBackupFile(pszFilePath);
		BAIL_ON_CENTERIS_ERROR(ceError);

		ceError = CTMoveFile(pszTmpPath, pszFilePath);
		BAIL_ON_CENTERIS_ERROR(ceError);

		bRemoveFile = FALSE;
	}

      error:

	regfree(&passwd_rx);

	regfree(&group_rx);

	if (fpSrc)
		fclose(fpSrc);

	if (fpDst)
		fclose(fpDst);

	if (bRemoveFile)
		CTRemoveFile(pszTmpPath);

	if (pszTmpPath)
		CTFreeString(pszTmpPath);

	return ceError;
}
