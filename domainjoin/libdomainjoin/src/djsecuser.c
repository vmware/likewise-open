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

static PCSTR USER_SECURITY_CONFIG_PATH = "/etc/security/user";

static
 CENTERROR MakeTmpUserSecurityPath(PCSTR pszPath, PSTR * ppszNewPath)
{
	CENTERROR ceError = CENTERROR_SUCCESS;
	PSTR pszNewPath = NULL;

	ceError = CTAllocateMemory(strlen(pszPath) + sizeof(".domainjoin"),
				   (PVOID *) & pszNewPath);
	BAIL_ON_CENTERIS_ERROR(ceError);

	sprintf(pszNewPath, "%s.domainjoin", pszPath);

	*ppszNewPath = pszNewPath;

	return ceError;

      error:

	if (pszNewPath)
		CTFreeMemory(pszNewPath);

	return ceError;
}

static BOOLEAN IsComment(PSTR pszLine)
{
	PSTR pszTmp = pszLine;

	if (IsNullOrEmptyString(pszLine))
		return TRUE;

	while (*pszTmp != '\0' && isspace(*pszTmp))
		pszTmp++;

	return *pszTmp == '*' || *pszTmp == '\0';
}

static BOOLEAN IsDefaultSection(PSTR pszLine)
{
	PSTR pszTmp = pszLine;

	if (IsNullOrEmptyString(pszLine))
		return FALSE;

	while (*pszTmp != '\0' && isspace(*pszTmp))
		pszTmp++;

	return !strncmp(pszTmp, "default:", sizeof("default:") - 1);
}

CENTERROR ConfigureUserSecurity(PCSTR pszConfigFilePath)
{
	CENTERROR ceError = CENTERROR_SUCCESS;
	BOOLEAN bFileExists = FALSE;
	PCSTR pszValueRegExp = "^[[:space:]]*SYSTEM[[:space:]]*=.*$";
	PCSTR pszSectionRegExp = "^[[:space:]]*.*:[[:space:]]*$";
	regex_t rx_val;
	regex_t rx_section;
	FILE *fpSrc = NULL;
	FILE *fpDst = NULL;
	PSTR pszDstPath = NULL;
	PSTR pszTmp = NULL;
	CHAR szBuf[1024 + 1];
	BOOLEAN bIsDefaultSection = FALSE;
	BOOLEAN bRemoveFile = FALSE;
	BOOLEAN bWriteOrig = FALSE;
	PCSTR pszFilePath = NULL;

	if (IsNullOrEmptyString(pszConfigFilePath))
		pszFilePath = USER_SECURITY_CONFIG_PATH;
	else
		pszFilePath = pszConfigFilePath;

	ceError = CTCheckFileExists(pszFilePath, &bFileExists);
	BAIL_ON_CENTERIS_ERROR(ceError);

	if (!bFileExists)
		goto done;

	memset(&rx_val, 0, sizeof(regex_t));
	memset(&rx_section, 0, sizeof(regex_t));

	if (regcomp(&rx_val, pszValueRegExp, REG_EXTENDED) < 0) {
		ceError = CENTERROR_REGEX_COMPILE_FAILED;
		BAIL_ON_CENTERIS_ERROR(ceError);
	}

	if (regcomp(&rx_section, pszSectionRegExp, REG_EXTENDED) < 0) {
		ceError = CENTERROR_REGEX_COMPILE_FAILED;
		BAIL_ON_CENTERIS_ERROR(ceError);
	}

	if ((fpSrc = fopen(pszFilePath, "r")) == NULL) {
		ceError = CTMapSystemError(errno);
		BAIL_ON_CENTERIS_ERROR(ceError);
	}

	ceError = MakeTmpUserSecurityPath(pszFilePath, &pszDstPath);
	BAIL_ON_CENTERIS_ERROR(ceError);

	if ((fpDst = fopen(pszDstPath, "w")) == NULL) {
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

		if (IsComment(szBuf)) {

			bWriteOrig = TRUE;

		} else if (!regexec(&rx_section, szBuf, (size_t) 0, NULL, 0)) {

			bIsDefaultSection = IsDefaultSection(szBuf);

			bWriteOrig = TRUE;

		} else if (!regexec(&rx_val, szBuf, (size_t) 0, NULL, 0)) {

			if (bIsDefaultSection) {

				if (strstr(szBuf, "LWIDENTITY")) {

					/* We already have the required setting */
					/* Nothing to do */
					goto done;
				}

				pszTmp = szBuf + strlen(szBuf) - 1;
				while (pszTmp && (pszTmp != szBuf)
				       && isspace(*pszTmp))
					pszTmp--;

				*pszTmp = '\0';

				fprintf(fpDst, "%s OR LWIDENTITY\"\n", szBuf);

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

	ceError = CTMoveFile(pszDstPath, pszFilePath);
	BAIL_ON_CENTERIS_ERROR(ceError);

	bRemoveFile = FALSE;

      done:
      error:

	regfree(&rx_val);

	regfree(&rx_section);

	if (fpSrc)
		fclose(fpSrc);

	if (fpDst)
		fclose(fpDst);

	if (bRemoveFile) {
		CTRemoveFile(pszDstPath);
	}

	if (pszDstPath)
		CTFreeString(pszDstPath);

	return ceError;
}

CENTERROR UnconfigureUserSecurity(PCSTR pszConfigFilePath)
{
	CENTERROR ceError = CENTERROR_SUCCESS;
	// TODO: Enable this in a later release
#if 0
	BOOLEAN bFileExists = FALSE;
	PCSTR pszValueRegExp = "^[[:space:]]*SYSTEM[[:space:]]*=.*$";
	PCSTR pszSectionRegExp = "^[[:space:]]*.*:[[:space:]]*$";
	regex_t rx_val;
	regex_t rx_section;
	FILE *fpSrc = NULL;
	FILE *fpDst = NULL;
	PSTR pszDstPath = NULL;
	PSTR pszFilePath = NULL;
	CHAR szBuf[1024 + 1];
	BOOLEAN bIsDefaultSection = FALSE;
	BOOLEAN bRemoveFile = FALSE;
	BOOLEAN bWriteOrig = FALSE;

	if (IsNullOrEmptyString(pszConfigFilePath))
		pszFilePath = USER_SECURITY_CONFIG_PATH;
	else
		pszFilePath = pszConfigFilePath;

	ceError = CTCheckFileExists(pszFilePath, &bFileExists);
	BAIL_ON_CENTERIS_ERROR(ceError);

	if (!bFileExists)
		goto done;

	memset(&rx_val, 0, sizeof(regex_t));
	memset(&rx_section, 0, sizeof(regex_t));

	if (regcomp(&rx_val, pszValueRegExp, REG_EXTENDED) < 0) {
		ceError = CENTERROR_REGEX_COMPILE_FAILED;
		BAIL_ON_CENTERIS_ERROR(ceError);
	}

	if (regcomp(&rx_section, pszSectionRegExp, REG_EXTENDED) < 0) {
		ceError = CENTERROR_REGEX_COMPILE_FAILED;
		BAIL_ON_CENTERIS_ERROR(ceError);
	}

	if ((fpSrc = fopen(pszFilePath, "r")) == NULL) {
		ceError = CTMapSystemError(errno);
		BAIL_ON_CENTERIS_ERROR(ceError);
	}

	ceError = MakeTmpUserSecurityPath(pszFilePath, &pszDstPath);
	BAIL_ON_CENTERIS_ERROR(ceError);

	if ((fpDst = fopen(pszDstPath, "w")) == NULL) {
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

		if (IsComment(szBuf)) {

			bWriteOrig = TRUE;

		} else if (!regexec(&rx_section, szBuf, (size_t) 0, NULL, 0)) {

			bIsDefaultSection = IsDefaultSection(szBuf);

			bWriteOrig = TRUE;

		} else if (!regexec(&rx_val, szBuf, (size_t) 0, NULL,)) {

			if (bIsDefaultSection) {

				if ((pszTmp =
				     strstr(szBuf, "LWIDENTITY")) == NULL) {

					/* We don't have the required setting */
					/* Nothing to do */
					goto done;
				}

				/* Start from before LWIDENTITY and skip spaces */
				pszBegin =
				    (pszTmp != szBuf ? pszTmp - 1 : szBuf);
				while (pszBegin != szBuf && isspace(*pszBegin))
					pszBegin--;

				/* Go back before the current clause */
				while (pszBegin != szBuf
				       && !isspace(*(pszBegin - 1)))
					pszBegin--;

				if (strncasecmp
				    (pszBegin, "OR", sizeof("OR") - 1)
				    && strncasecmp(pszBegin, "AND",
						   sizeof("AND") - 1)) {
					pszBegin = pszTmp;
				}

				pszTmp += sizeof("LWIDENTITY") - 1;
				while (*pszTmp) {
					*pszBegin++ = *pszTmp++;
				}
				*pszBegin = '\0';

				fprintf(fpDst, "%s", szBuf);

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

	if (fpSrc) {
		fclose(fpSrc);
		fpSrc = NULL;
	}

	if (fpDst) {
		fclose(fpDst);
		fpDst = NULL;
	}

	ceError = CTMoveFile(pszDstPath, pszFilePath);
	BAIL_ON_CENTERIS_ERROR(ceError);

	bRemoveFile = FALSE;

      done:
      error:

	regfree(&rx_val);

	regfree(&rx_section);

	if (fpSrc)
		fclose(fpSrc);

	if (fpDst)
		fclose(fpDst);

	if (bRemoveFile) {
		CTRemoveFile(pszDstPath);
	}

	if (pszDstPath)
		CTFreeString(pszDstPath);

#endif

	return ceError;
}

#endif
