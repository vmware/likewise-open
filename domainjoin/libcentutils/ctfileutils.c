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

/* ex: set tabstop=4 expandtab shiftwidth=4: */
#include "ctbase.h"
#include "unistd.h"

CENTERROR CTRemoveFile(PCSTR pszPath)
{
	CENTERROR ceError = CENTERROR_SUCCESS;

	while (1) {
		if (unlink(pszPath) < 0) {
			if (errno == EINTR) {
				continue;
			}
			ceError = CTMapSystemError(errno);
			BAIL_ON_CENTERIS_ERROR(ceError);
		} else {
			break;
		}
	}

      error:

	return ceError;
}

/*
// TODO: Check access and removability before actual deletion
*/
CENTERROR CTRemoveDirectory(PCSTR pszPath)
{
	CENTERROR ceError = CENTERROR_SUCCESS;
	DIR *pDir = NULL;
	struct dirent *pDirEntry = NULL;
	struct stat statbuf;
	CHAR szBuf[PATH_MAX + 1];

	if ((pDir = opendir(pszPath)) == NULL) {
		ceError = CTMapSystemError(errno);
		BAIL_ON_CENTERIS_ERROR(ceError);
	}

	while ((pDirEntry = readdir(pDir)) != NULL) {

		if (!strcmp(pDirEntry->d_name, "..") ||
		    !strcmp(pDirEntry->d_name, "."))
			continue;

		sprintf(szBuf, "%s/%s", pszPath, pDirEntry->d_name);

		memset(&statbuf, 0, sizeof(struct stat));

		if (stat(szBuf, &statbuf) < 0) {
			ceError = CTMapSystemError(errno);
			BAIL_ON_CENTERIS_ERROR(ceError);
		}

		if ((statbuf.st_mode & S_IFMT) == S_IFDIR) {
			ceError = CTRemoveDirectory(szBuf);
			BAIL_ON_CENTERIS_ERROR(ceError);

			if (rmdir(szBuf) < 0) {
				ceError = CTMapSystemError(ceError);
				BAIL_ON_CENTERIS_ERROR(ceError);
			}

		} else {

			ceError = CTRemoveFile(szBuf);
			BAIL_ON_CENTERIS_ERROR(ceError);

		}
	}

      error:

	if (pDir)
		closedir(pDir);

	return ceError;
}

CENTERROR CTCheckLinkExists(PCSTR pszPath, PBOOLEAN pbLinkExists)
{
	CENTERROR ceError = CENTERROR_SUCCESS;

	struct stat statbuf;

	memset(&statbuf, 0, sizeof(struct stat));

	while (1) {
		if (lstat(pszPath, &statbuf) < 0) {
			if (errno == EINTR) {
				continue;
			} else if (errno == ENOENT || errno == ENOTDIR) {
				*pbLinkExists = FALSE;
				break;
			}
			ceError = CTMapSystemError(errno);
			BAIL_ON_CENTERIS_ERROR(ceError);
		} else {
			*pbLinkExists =
			    (((statbuf.st_mode & S_IFMT) ==
			      S_IFLNK) ? TRUE : FALSE);
			break;
		}
	}

      error:

	return ceError;
}

CENTERROR CTCheckSockExists(PCSTR pszPath, PBOOLEAN pbSockExists)
{
	CENTERROR ceError = CENTERROR_SUCCESS;

	struct stat statbuf;

	memset(&statbuf, 0, sizeof(struct stat));

	while (1) {
		if (stat(pszPath, &statbuf) < 0) {
			if (errno == EINTR) {
				continue;
			} else if (errno == ENOENT || errno == ENOTDIR) {
				*pbSockExists = FALSE;
				break;
			}
			ceError = CTMapSystemError(errno);
			BAIL_ON_CENTERIS_ERROR(ceError);
		} else {
			*pbSockExists =
			    (((statbuf.st_mode & S_IFMT) ==
			      S_IFSOCK) ? TRUE : FALSE);
			break;
		}
	}

      error:

	return ceError;
}

CENTERROR CTCheckFileExists(PCSTR pszPath, PBOOLEAN pbFileExists)
{
	CENTERROR ceError = CENTERROR_SUCCESS;

	struct stat statbuf;

	memset(&statbuf, 0, sizeof(struct stat));

	while (1) {
		if (stat(pszPath, &statbuf) < 0) {
			if (errno == EINTR) {
				continue;
			} else if (errno == ENOENT || errno == ENOTDIR) {
				*pbFileExists = FALSE;
				break;
			}
			ceError = CTMapSystemError(errno);
			BAIL_ON_CENTERIS_ERROR(ceError);
		} else {
			*pbFileExists =
			    (((statbuf.st_mode & S_IFMT) ==
			      S_IFREG) ? TRUE : FALSE);
			break;
		}
	}

      error:

	return ceError;
}

CENTERROR CTCheckDirectoryExists(PCSTR pszPath, PBOOLEAN pbDirExists)
{
	CENTERROR ceError = CENTERROR_SUCCESS;

	struct stat statbuf;

	while (1) {

		memset(&statbuf, 0, sizeof(struct stat));

		if (stat(pszPath, &statbuf) < 0) {

			if (errno == EINTR) {
				continue;
			} else if (errno == ENOENT || errno == ENOTDIR) {
				*pbDirExists = FALSE;
				break;
			}
			ceError = CTMapSystemError(errno);
			BAIL_ON_CENTERIS_ERROR(ceError);

		}

		/*
		   The path exists. Is it a directory?
		 */

		*pbDirExists =
		    (((statbuf.st_mode & S_IFMT) == S_IFDIR) ? TRUE : FALSE);
		break;
	}

      error:

	return ceError;
}

CENTERROR CTMoveFile(PCSTR pszSrcPath, PCSTR pszDstPath)
{
	CENTERROR ceError = CENTERROR_SUCCESS;

	if (rename(pszSrcPath, pszDstPath) < 0) {
		ceError = CTMapSystemError(errno);
	}

	return ceError;
}

CENTERROR
CTCopyFileWithPerms(PCSTR pszSrcPath, PCSTR pszDstPath, mode_t dwPerms)
{
	CENTERROR ceError = CENTERROR_SUCCESS;
	PCSTR pszTmpSuffix = ".tmp_likewise";
	PSTR pszTmpPath = NULL;
	BOOLEAN bRemoveFile = FALSE;
	CHAR szBuf[1024 + 1];
	int iFd = -1;
	int oFd = -1;
	DWORD dwBytesRead = 0;

	if (IsNullOrEmptyString(pszSrcPath) || IsNullOrEmptyString(pszDstPath)) {
		ceError = CENTERROR_INVALID_PARAMETER;
		BAIL_ON_CENTERIS_ERROR(ceError);
	}

	ceError =
	    CTAllocateMemory(strlen(pszDstPath) + strlen(pszTmpSuffix) + 2,
			     (PVOID *) & pszTmpPath);
	BAIL_ON_CENTERIS_ERROR(ceError);

	strcpy(pszTmpPath, pszDstPath);
	strcat(pszTmpPath, pszTmpSuffix);

	if ((iFd = open(pszSrcPath, O_RDONLY, S_IRUSR)) < 0) {
		ceError = CTMapSystemError(errno);
		BAIL_ON_CENTERIS_ERROR(ceError);
	}

	if ((oFd =
	     open(pszTmpPath, O_WRONLY | O_TRUNC | O_CREAT,
		  S_IRUSR | S_IWUSR)) < 0) {
		ceError = CTMapSystemError(errno);
		BAIL_ON_CENTERIS_ERROR(ceError);
	}

	bRemoveFile = TRUE;

	while (1) {

		if ((dwBytesRead = read(iFd, szBuf, 1024)) < 0) {

			if (errno == EINTR)
				continue;

			ceError = CTMapSystemError(errno);
			BAIL_ON_CENTERIS_ERROR(ceError);
		}

		if (dwBytesRead == 0)
			break;

		if (write(oFd, szBuf, dwBytesRead) != dwBytesRead) {

			if (errno == EINTR)
				continue;

			ceError = CTMapSystemError(errno);
			BAIL_ON_CENTERIS_ERROR(ceError);
		}

	}

	close(iFd);
	iFd = -1;
	close(oFd);
	oFd = -1;

	ceError = CTMoveFile(pszTmpPath, pszDstPath);
	BAIL_ON_CENTERIS_ERROR(ceError);

	bRemoveFile = FALSE;

	ceError = CTChangePermissions(pszDstPath, dwPerms);
	BAIL_ON_CENTERIS_ERROR(ceError);

      error:

	if (iFd >= 0)
		close(iFd);

	if (oFd >= 0)
		close(oFd);

	if (bRemoveFile) {
		CTRemoveFile(pszTmpPath);
	}

	if (pszTmpPath)
		CTFreeString(pszTmpPath);

	return ceError;
}

CENTERROR
CTGetOwnerAndPermissions(PCSTR pszSrcPath,
			 uid_t * uid, gid_t * gid, mode_t * mode)
{
	CENTERROR ceError = CENTERROR_SUCCESS;
	struct stat statbuf;

	memset(&statbuf, 0, sizeof(struct stat));

	if (stat(pszSrcPath, &statbuf) < 0) {
		ceError = CTMapSystemError(errno);
		BAIL_ON_CENTERIS_ERROR(ceError);
	}

	*uid = statbuf.st_uid;
	*gid = statbuf.st_gid;
	*mode = statbuf.st_mode;

      error:

	return ceError;
}

CENTERROR CTCheckFileOrLinkExists(PCSTR pszPath, PBOOLEAN pbExists)
{
	CENTERROR ceError = CENTERROR_SUCCESS;
	BAIL_ON_CENTERIS_ERROR(ceError = CTCheckFileExists(pszPath, pbExists));
	if (*pbExists == TRUE)
		goto error;

	BAIL_ON_CENTERIS_ERROR(ceError = CTCheckLinkExists(pszPath, pbExists));

      error:
	return ceError;
}

CENTERROR CTGetFileTimeStamps(PCSTR pszFilePath, time_t * patime,	/* time of last access */
			      time_t * pmtime,	/* time of last modification */
			      time_t * pctime)
{				/* time of last status change */
	CENTERROR ceError = CENTERROR_SUCCESS;
	struct stat statbuf;

	memset(&statbuf, 0, sizeof(struct stat));

	if (stat(pszFilePath, &statbuf) < 0) {
		ceError = CTMapSystemError(errno);
		BAIL_ON_CENTERIS_ERROR(ceError);
	}

	if (patime) {
		*patime = statbuf.st_atime;
	}

	if (pmtime) {
		*pmtime = statbuf.st_mtime;
	}

	if (pctime) {
		*pctime = statbuf.st_ctime;
	}
      error:

	return ceError;
}

CENTERROR CTCopyFileWithOriginalPerms(PCSTR pszSrcPath, PCSTR pszDstPath)
{
	CENTERROR ceError = CENTERROR_SUCCESS;
	struct stat statbuf;

	memset(&statbuf, 0, sizeof(struct stat));

	if (stat(pszSrcPath, &statbuf) < 0) {
		ceError = CTMapSystemError(errno);
		BAIL_ON_CENTERIS_ERROR(ceError);
	}

	ceError = CTCopyFileWithPerms(pszSrcPath, pszDstPath, statbuf.st_mode);
	BAIL_ON_CENTERIS_ERROR(ceError);

      error:

	return ceError;
}

CENTERROR CTChangePermissions(PCSTR pszPath, mode_t dwFileMode)
{
	CENTERROR ceError = CENTERROR_SUCCESS;

	while (1) {
		if (chmod(pszPath, dwFileMode) < 0) {
			if (errno == EINTR) {
				continue;
			}
			ceError = CTMapSystemError(errno);
			BAIL_ON_CENTERIS_ERROR(ceError);
		} else {
			break;
		}
	}

      error:

	return ceError;
}

CENTERROR CTChangeOwner(PCSTR pszPath, uid_t uid, gid_t gid)
{
	CENTERROR ceError = CENTERROR_SUCCESS;

	while (1) {
		if (chown(pszPath, uid, gid) < 0) {
			if (errno == EINTR) {
				continue;
			}
			ceError = CTMapSystemError(errno);
			BAIL_ON_CENTERIS_ERROR(ceError);
		} else {
			break;
		}
	}

      error:

	return ceError;
}

CENTERROR
CTChangeOwnerAndPermissions(PCSTR pszPath,
			    uid_t uid, gid_t gid, mode_t dwFileMode)
{
	CENTERROR ceError = CENTERROR_SUCCESS;

	ceError = CTChangeOwner(pszPath, uid, gid);
	BAIL_ON_CENTERIS_ERROR(ceError);

	ceError = CTChangePermissions(pszPath, dwFileMode);
	BAIL_ON_CENTERIS_ERROR(ceError);

      error:

	return ceError;
}

CENTERROR CTGetCurrentDirectoryPath(PSTR * ppszPath)
{
	CENTERROR ceError = CENTERROR_SUCCESS;
	CHAR szBuf[PATH_MAX + 1];
	PSTR pszPath = NULL;

	if (getcwd(szBuf, PATH_MAX) == NULL) {
		ceError = CTMapSystemError(errno);
		BAIL_ON_CENTERIS_ERROR(ceError);
	}

	ceError = CTAllocateString(szBuf, &pszPath);
	BAIL_ON_CENTERIS_ERROR(ceError);

	*ppszPath = pszPath;

	return ceError;

      error:

	if (pszPath) {
		CTFreeString(pszPath);
	}

	return ceError;
}

CENTERROR CTChangeDirectory(PSTR pszPath)
{
	if (pszPath == NULL || *pszPath == '\0')
		return CENTERROR_INVALID_PARAMETER;

	if (chdir(pszPath) < 0)
		return CTMapSystemError(errno);

	return CENTERROR_SUCCESS;
}

CENTERROR
CTCreateDirectoryRecursive(PSTR pszCurDirPath,
			   PSTR pszTmpPath,
			   PSTR * ppszTmp,
			   DWORD dwFileMode, DWORD dwWorkingFileMode, int iPart)
{
	CENTERROR ceError = CENTERROR_SUCCESS;
	PSTR pszDirPath = NULL;
	BOOLEAN bDirCreated = FALSE;
	BOOLEAN bDirExists = FALSE;
	CHAR szDelimiters[] = "/";

	PSTR pszToken =
	    strtok_r((iPart ? NULL : pszTmpPath), szDelimiters, ppszTmp);

	if (pszToken != NULL) {

		ceError =
		    CTAllocateMemory(strlen(pszCurDirPath) + strlen(pszToken) +
				     2, (PVOID *) & pszDirPath);
		BAIL_ON_CENTERIS_ERROR(ceError);

		sprintf(pszDirPath,
			"%s/%s",
			(!strcmp(pszCurDirPath, "/") ? "" : pszCurDirPath),
			pszToken);

		ceError = CTCheckDirectoryExists(pszDirPath, &bDirExists);
		BAIL_ON_CENTERIS_ERROR(ceError);

		if (!bDirExists) {
			if (mkdir(pszDirPath, dwWorkingFileMode) < 0) {
				ceError = CTMapSystemError(errno);
				BAIL_ON_CENTERIS_ERROR(ceError);
			}
			bDirCreated = TRUE;
		}

		ceError = CTChangeDirectory(pszDirPath);
		BAIL_ON_CENTERIS_ERROR(ceError);

		ceError = CTCreateDirectoryRecursive(pszDirPath,
						     pszTmpPath,
						     ppszTmp,
						     dwFileMode,
						     dwWorkingFileMode,
						     iPart + 1);
		BAIL_ON_CENTERIS_ERROR(ceError);
	}

	if (bDirCreated && (dwFileMode != dwWorkingFileMode)) {
		ceError = CTChangePermissions(pszDirPath, dwFileMode);
		BAIL_ON_CENTERIS_ERROR(ceError);
	}

	if (pszDirPath) {
		CTFreeMemory(pszDirPath);
	}

	return ceError;

      error:

	if (bDirCreated) {
		CTRemoveDirectory(pszDirPath);
	}

	if (pszDirPath) {
		CTFreeMemory(pszDirPath);
	}

	return ceError;
}

CENTERROR CTCreateDirectory(PSTR pszPath, mode_t dwFileMode)
{
	CENTERROR ceError = CENTERROR_SUCCESS;
	PSTR pszCurDirPath = NULL;
	PSTR pszTmpPath = NULL;
	PSTR pszTmp = NULL;
	mode_t dwWorkingFileMode;

	if (pszPath == NULL || *pszPath == '\0') {
		ceError = CENTERROR_INVALID_PARAMETER;
		BAIL_ON_CENTERIS_ERROR(ceError);
	}

	dwWorkingFileMode = dwFileMode;
	if (!(dwFileMode & S_IXUSR)) {
		/*
		 * This is so that we can navigate the folders
		 * when we are creating the subfolders
		 */
		dwWorkingFileMode |= S_IXUSR;
	}

	ceError = CTGetCurrentDirectoryPath(&pszCurDirPath);
	BAIL_ON_CENTERIS_ERROR(ceError);

	ceError = CTAllocateString(pszPath, &pszTmpPath);
	BAIL_ON_CENTERIS_ERROR(ceError);

	if (*pszPath == '/') {
		ceError = CTChangeDirectory("/");
		BAIL_ON_CENTERIS_ERROR(ceError);

		ceError =
		    CTCreateDirectoryRecursive("/", pszTmpPath, &pszTmp,
					       dwFileMode, dwWorkingFileMode,
					       0);
		BAIL_ON_CENTERIS_ERROR(ceError);

	} else {

		ceError =
		    CTCreateDirectoryRecursive(pszCurDirPath, pszTmpPath,
					       &pszTmp, dwFileMode,
					       dwWorkingFileMode, 0);
		BAIL_ON_CENTERIS_ERROR(ceError);

	}

      error:

	if (pszCurDirPath) {

		CTChangeDirectory(pszCurDirPath);

		CTFreeMemory(pszCurDirPath);

	}

	if (pszTmpPath) {
		CTFreeMemory(pszTmpPath);
	}

	return ceError;
}

static
    CENTERROR
CTGetMatchingDirEntryPathsInFolder(PSTR pszDirPath,
				   PSTR pszFileNameRegExp,
				   PSTR ** pppszHostFilePaths,
				   PDWORD pdwNPaths, DWORD dirEntryType)
{
	typedef struct __PATHNODE {
		PSTR pszPath;
		struct __PATHNODE *pNext;
	} PATHNODE, *PPATHNODE;

	CENTERROR ceError = CENTERROR_SUCCESS;
	DIR *pDir = NULL;
	struct dirent *pDirEntry = NULL;
	regex_t rx;
	regmatch_t *pResult = NULL;
	size_t nMatch = 1;
	DWORD dwNPaths = 0;
	DWORD iPath = 0;
	PSTR *ppszHostFilePaths = NULL;
	CHAR szBuf[PATH_MAX + 1];
	struct stat statbuf;
	PPATHNODE pPathList = NULL;
	PPATHNODE pPathNode = NULL;

	if (regcomp(&rx, pszFileNameRegExp, REG_EXTENDED) != 0) {
		ceError = CENTERROR_REGEX_COMPILE_FAILED;
		BAIL_ON_CENTERIS_ERROR(ceError);
	}

	ceError = CTAllocateMemory(sizeof(regmatch_t), (PVOID *) & pResult);
	BAIL_ON_CENTERIS_ERROR(ceError);

	pDir = opendir(pszDirPath);
	if (pDir == NULL) {
		ceError = CTMapSystemError(errno);
		BAIL_ON_CENTERIS_ERROR(ceError);
	}

	while ((pDirEntry = readdir(pDir)) != NULL) {

		sprintf(szBuf, "%s/%s", pszDirPath, pDirEntry->d_name);
		memset(&statbuf, 0, sizeof(struct stat));
		if (stat(szBuf, &statbuf) < 0) {
			ceError = CTMapSystemError(errno);
			BAIL_ON_CENTERIS_ERROR(ceError);
		}
		/*
		 * For now, we are searching only for regular files
		 * This may be enhanced in the future to support additional
		 * file system entry types
		 */
		if (((statbuf.st_mode & S_IFMT) == dirEntryType) &&
		    (regexec(&rx, pDirEntry->d_name, nMatch, pResult, 0) ==
		     0)) {
			dwNPaths++;

			ceError =
			    CTAllocateMemory(sizeof(PATHNODE),
					     (PVOID *) & pPathNode);
			BAIL_ON_CENTERIS_ERROR(ceError);

			ceError = CTAllocateString(szBuf, &pPathNode->pszPath);
			BAIL_ON_CENTERIS_ERROR(ceError);

			pPathNode->pNext = pPathList;
			pPathList = pPathNode;
			pPathNode = NULL;
		}
	}

	if (pPathList) {
		ceError = CTAllocateMemory(sizeof(PSTR) * dwNPaths,
					   (PVOID *) & ppszHostFilePaths);
		BAIL_ON_CENTERIS_ERROR(ceError);
		/*
		 *  The linked list is in reverse.
		 *  Assign values in reverse to get it right
		 */
		iPath = dwNPaths - 1;
		pPathNode = pPathList;
		while (pPathNode) {
			*(ppszHostFilePaths + iPath) = pPathNode->pszPath;
			pPathNode->pszPath = NULL;
			pPathNode = pPathNode->pNext;
			iPath--;
		}
	}

	*pppszHostFilePaths = ppszHostFilePaths;
	ppszHostFilePaths = NULL;

	*pdwNPaths = dwNPaths;

	if (pPathNode) {
		pPathNode->pNext = pPathList;
		pPathList = pPathNode;
	}

	while (pPathList) {
		pPathNode = pPathList;
		pPathList = pPathList->pNext;
		if (pPathNode->pszPath)
			CTFreeString(pPathNode->pszPath);
		CTFreeMemory(pPathNode);
	}

	regfree(&rx);

	if (pResult) {
		CTFreeMemory(pResult);
	}

	if (pDir)
		closedir(pDir);

	return ceError;

      error:

	*pdwNPaths = 0;
	*pppszHostFilePaths = NULL;

	if (ppszHostFilePaths) {
		CTFreeStringArray(ppszHostFilePaths, dwNPaths);
	}

	if (pPathNode) {
		pPathNode->pNext = pPathList;
		pPathList = pPathNode;
	}

	while (pPathList) {
		pPathNode = pPathList;
		pPathList = pPathList->pNext;
		if (pPathNode->pszPath)
			CTFreeString(pPathNode->pszPath);
		CTFreeMemory(pPathNode);
	}

	regfree(&rx);

	if (pResult) {
		CTFreeMemory(pResult);
	}

	if (pDir)
		closedir(pDir);

	return ceError;
}

CENTERROR
CTGetMatchingFilePathsInFolder(PSTR pszDirPath,
			       PSTR pszFileNameRegExp,
			       PSTR ** pppszHostFilePaths, PDWORD pdwNPaths)
{
	return CTGetMatchingDirEntryPathsInFolder(pszDirPath,
						  pszFileNameRegExp,
						  pppszHostFilePaths,
						  pdwNPaths, S_IFREG);
}

CENTERROR
CTGetMatchingDirPathsInFolder(PSTR pszDirPath,
			      PSTR pszDirNameRegExp,
			      PSTR ** pppszHostDirPaths, PDWORD pdwNPaths)
{
	return CTGetMatchingDirEntryPathsInFolder(pszDirPath,
						  pszDirNameRegExp,
						  pppszHostDirPaths,
						  pdwNPaths, S_IFDIR);
}

CENTERROR
CTCheckFileHoldsPattern(PCSTR pszFilePath,
			PCSTR pszPattern, PBOOLEAN pbPatternExists)
{
	CENTERROR ceError = CENTERROR_SUCCESS;
	FILE *fp = NULL;
	CHAR szBuf[1024 + 1];
	regex_t rx;
	regmatch_t *pResult = NULL;
	size_t nMatch = 1;
	BOOLEAN bPatternExists = FALSE;

	memset(&rx, 0, sizeof(regex_t));

	if (regcomp(&rx, pszPattern, REG_EXTENDED) != 0) {
		ceError = CENTERROR_REGEX_COMPILE_FAILED;
		BAIL_ON_CENTERIS_ERROR(ceError);
	}

	ceError = CTAllocateMemory(sizeof(regmatch_t), (PVOID *) & pResult);
	BAIL_ON_CENTERIS_ERROR(ceError);

	fp = fopen(pszFilePath, "r");
	if (fp == NULL) {
		ceError = CTMapSystemError(errno);
		BAIL_ON_CENTERIS_ERROR(ceError);
	}

	while (!feof(fp)) {
		if (fgets(szBuf, 1024, fp)) {
			if (regexec(&rx, szBuf, nMatch, pResult, 0) == 0) {
				bPatternExists = TRUE;
				break;
			}
		} else if (!feof(fp)) {
			ceError = CTMapSystemError(errno);
			BAIL_ON_CENTERIS_ERROR(ceError);
		}
	}

	*pbPatternExists = bPatternExists;

      error:

	regfree(&rx);

	if (pResult)
		CTFreeMemory(pResult);

	if (fp != NULL)
		fclose(fp);

	return ceError;
}

CENTERROR CTGetAbsolutePath(PSTR pszRelativePath, PSTR * ppszAbsolutePath)
{
	CENTERROR ceError = CENTERROR_SUCCESS;
	CHAR szBuf[PATH_MAX + 1];

	if (realpath(pszRelativePath, szBuf) == NULL) {
		ceError = CTMapSystemError(errno);
		BAIL_ON_CENTERIS_ERROR(ceError);
	}

	ceError = CTAllocateString(szBuf, ppszAbsolutePath);
	BAIL_ON_CENTERIS_ERROR(ceError);

      error:

	return ceError;
}

CENTERROR CTRemoveFiles(PSTR pszPath, BOOLEAN fDirectory, BOOLEAN fRecursive)
{
	CENTERROR ceError = CENTERROR_SUCCESS;
	CHAR szCommand[2 * PATH_MAX + 1];

	sprintf(szCommand, "/bin/rm -f %s %s %s",
		fDirectory ? "-d" : "", fRecursive ? "-r" : "", pszPath);

	if (system(szCommand) < 0) {
		ceError = CTMapSystemError(errno);
		BAIL_ON_CENTERIS_ERROR(ceError);
	}

      error:

	return (ceError);
}

CENTERROR CTGetSymLinkTarget(PCSTR pszPath, PSTR * ppszTargetPath)
{
	CENTERROR ceError = CENTERROR_SUCCESS;
	CHAR szBuf[PATH_MAX + 1];

	memset(szBuf, 0, PATH_MAX);

	while (1) {

		if (readlink(pszPath, szBuf, PATH_MAX) < 0) {
			if (errno == EINTR)
				continue;

			ceError = CTMapSystemError(errno);
			BAIL_ON_CENTERIS_ERROR(ceError);
		}

		break;
	}

	ceError = CTAllocateString(szBuf, ppszTargetPath);
	BAIL_ON_CENTERIS_ERROR(ceError);

      error:

	return ceError;
}

CENTERROR CTCreateSymLink(PCSTR pszOldPath, PCSTR pszNewPath)
{
	CENTERROR ceError = CENTERROR_SUCCESS;

	if (IsNullOrEmptyString(pszOldPath) || IsNullOrEmptyString(pszNewPath)) {
		ceError = CENTERROR_INVALID_PARAMETER;
		BAIL_ON_CENTERIS_ERROR(ceError);
	}

	if (symlink(pszOldPath, pszNewPath) < 0) {
		ceError = CTMapSystemError(errno);
		BAIL_ON_CENTERIS_ERROR(ceError);
	}

      error:

	return ceError;
}

CENTERROR CTBackupFile(PCSTR path)
{
	CENTERROR ceError;
	PSTR backupPath = NULL;
	BOOLEAN exists;

	ceError = CTCheckFileExists(path, &exists);
	CLEANUP_ON_CENTERROR(ceError);
	if (!exists) {
		/* Do not need to backup, since the file does not yet exist. */
		goto cleanup;
	}

	ceError =
	    CTAllocateStringPrintf(&backupPath, "%s.lwidentity.orig", path);
	CLEANUP_ON_CENTERROR(ceError);

	ceError = CTCheckFileExists(backupPath, &exists);
	CLEANUP_ON_CENTERROR(ceError);

	if (exists) {
		CTFreeMemory(backupPath);
		backupPath = NULL;

		ceError =
		    CTAllocateStringPrintf(&backupPath, "%s.lwidentity.bak",
					   path);
		CLEANUP_ON_CENTERROR(ceError);
	}

	ceError = CTCopyFileWithOriginalPerms(path, backupPath);
	CLEANUP_ON_CENTERROR(ceError);

      cleanup:
	if (backupPath) {
		CTFreeMemory(backupPath);
		backupPath = NULL;
	}
	return ceError;
}

/*
// On some unix systems, you may not be allowed to
// move files across devices. So, we copy the file to a
// tmp file and then move it to the target location -
// and remove the original file
*/
CENTERROR CTMoveFileAcrossDevices(PCSTR pszSrcPath, PCSTR pszDstPath)
{
	CENTERROR ceError = CENTERROR_SUCCESS;
	CHAR szTmpPath[PATH_MAX + 1] = "";
	BOOLEAN bRemoveFile = FALSE;

	sprintf(szTmpPath, "%s_lwi_.tmp", pszDstPath);

	ceError = CTCopyFileWithOriginalPerms(pszSrcPath, szTmpPath);
	BAIL_ON_CENTERIS_ERROR(ceError);

	bRemoveFile = TRUE;

	ceError = CTMoveFile(szTmpPath, pszDstPath);
	BAIL_ON_CENTERIS_ERROR(ceError);

	bRemoveFile = FALSE;

	ceError = CTRemoveFile(pszSrcPath);
	BAIL_ON_CENTERIS_ERROR(ceError);

      error:

	if (bRemoveFile && !IsNullOrEmptyString(szTmpPath))
		CTRemoveFile(szTmpPath);

	return ceError;
}

/*
// Do not use this function on very large files since it reads the
// entire file into memory.
*/
CENTERROR CTReadFile(PCSTR pszFilePath, PSTR * ppBuffer, PLONG pSize)
{
	CENTERROR ceError = CENTERROR_SUCCESS;
	struct stat statbuf;
	FILE *fp = NULL;

	*ppBuffer = NULL;
	*pSize = 0;

	memset(&statbuf, 0, sizeof(struct stat));

	if (stat(pszFilePath, &statbuf) < 0) {
		ceError = CTMapSystemError(errno);
		BAIL_ON_CENTERIS_ERROR(ceError);
	}

	if (statbuf.st_size > 0) {
		/* allocate one additional byte of memory and set it to NULL to
		   allow for functions like strtok() to work properly */
		ceError =
		    CTAllocateMemory(statbuf.st_size + 1, (VOID *) ppBuffer);
		BAIL_ON_CENTERIS_ERROR(ceError);

		fp = fopen(pszFilePath, "r");

		if (fp == NULL) {
			ceError = CENTERROR_GP_FILE_OPEN_FAILED;
			BAIL_ON_CENTERIS_ERROR(ceError);
		}

		if (fread(*ppBuffer, statbuf.st_size, 1, fp) != 1) {
			ceError = CTMapSystemError(errno);
			BAIL_ON_CENTERIS_ERROR(ceError);
		}

		fclose(fp);
		fp = NULL;

		*pSize = statbuf.st_size;
	}

	return ceError;

      error:
	if (*ppBuffer) {
		CTFreeMemory(*ppBuffer);
		*ppBuffer = NULL;
	}

	if (fp) {
		fclose(fp);
	}

	return ceError;
}

CENTERROR CTOpenFile(PCSTR path, PCSTR mode, FILE ** handle)
{
	CENTERROR ceError = CENTERROR_SUCCESS;

	*handle = fopen(path, mode);

	if (!*handle) {
		ceError = CTMapSystemError(errno);
		BAIL_ON_CENTERIS_ERROR(ceError);
	}
      error:

	return ceError;
}

CENTERROR CTFileStreamWrite(FILE * handle, PCSTR data, unsigned int size)
{
	CENTERROR ceError = CENTERROR_SUCCESS;

	unsigned int written = 0;

	while (written < size) {
		int amount = fwrite(data + written, 1, size - written, handle);
		if (amount < 0) {
			ceError = CTMapSystemError(errno);
			BAIL_ON_CENTERIS_ERROR(ceError);
		}
		written += amount;
	}
      error:

	return ceError;
}

CENTERROR CTFilePrintf(FILE * handle, PCSTR format, ...
    )
{
	CENTERROR ceError = CENTERROR_SUCCESS;
	va_list args;
	int count;

	va_start(args, format);
	count = vfprintf(handle, format, args);
	va_end(args);

	if (count < 0) {
		ceError = CTMapSystemError(errno);
		BAIL_ON_CENTERIS_ERROR(ceError);
	}

      error:
	return ceError;
}

CENTERROR CTCloseFile(FILE * handle)
{
	CENTERROR ceError = CENTERROR_SUCCESS;

	if (fclose(handle)) {
		ceError = CTMapSystemError(errno);
		BAIL_ON_CENTERIS_ERROR(ceError);
	}

      error:

	return ceError;
}

CENTERROR
CTFileContentsSame(PCSTR pszFilePath1, PCSTR pszFilePath2, PBOOLEAN pbSame)
{
	CENTERROR ceError = CENTERROR_SUCCESS;
	FILE *fp1 = NULL;
	FILE *fp2 = NULL;
	BOOLEAN f1IsFile, f1IsLink;
	BOOLEAN f2IsFile, f2IsLink;
	unsigned char buffer1[1024], buffer2[1024];
	size_t read1, read2;

	ceError = CTCheckFileExists(pszFilePath1, &f1IsFile);
	BAIL_ON_CENTERIS_ERROR(ceError);

	ceError = CTCheckLinkExists(pszFilePath1, &f1IsLink);
	BAIL_ON_CENTERIS_ERROR(ceError);

	ceError = CTCheckFileExists(pszFilePath2, &f2IsFile);
	BAIL_ON_CENTERIS_ERROR(ceError);

	ceError = CTCheckLinkExists(pszFilePath2, &f2IsLink);
	BAIL_ON_CENTERIS_ERROR(ceError);

	f1IsFile |= f1IsLink;
	f2IsFile |= f2IsLink;

	if (!f1IsFile && !f2IsFile) {
		*pbSame = TRUE;
		goto error;
	}

	if (!f1IsFile || !f2IsFile) {
		*pbSame = FALSE;
		goto error;
	}

	ceError = CTOpenFile(pszFilePath1, "r", &fp1);
	BAIL_ON_CENTERIS_ERROR(ceError);
	ceError = CTOpenFile(pszFilePath2, "r", &fp2);
	BAIL_ON_CENTERIS_ERROR(ceError);

	while (1) {
		read1 = fread(buffer1, 1, sizeof(buffer1), fp1);
		if (read1 < sizeof(buffer1) && ferror(fp1)) {
			ceError = CTMapSystemError(errno);
			BAIL_ON_CENTERIS_ERROR(ceError);
		}

		read2 = fread(buffer2, 1, sizeof(buffer2), fp2);
		if (read2 < sizeof(buffer2) && ferror(fp2)) {
			ceError = CTMapSystemError(errno);
			BAIL_ON_CENTERIS_ERROR(ceError);
		}

		if (read1 != read2 || memcmp(buffer1, buffer2, read1) != 0) {
			*pbSame = FALSE;
			goto error;
		}

		if (read1 == 0)
			break;
	}

	*pbSame = TRUE;

      error:
	if (fp1 != NULL)
		fclose(fp1);
	if (fp2 != NULL)
		fclose(fp2);

	return ceError;
}

CENTERROR
CTRunSedOnFile(PCSTR pszSrcPath,
	       PCSTR pszDstPath, BOOLEAN bDashN, PCSTR pszExpression)
{
	CENTERROR ceError = CENTERROR_SUCCESS;
	PCSTR ppszArgs[] = { NULL,
		NULL,
		NULL,
		NULL,
	};
	int dwFdIn = -1, dwFdOut = -1;
	int argPos = 0;
	PPROCINFO pProcInfo = NULL;
	LONG status = 0;
	PSTR tempPath = NULL;
	uid_t uid;
	gid_t gid;
	mode_t mode;
	BOOLEAN sedFound;
	BOOLEAN isSame;

	BAIL_ON_CENTERIS_ERROR(ceError =
			       CTCheckFileOrLinkExists("/bin/sed", &sedFound));
	if (sedFound)
		ppszArgs[argPos++] = "/bin/sed";
	else {
		BAIL_ON_CENTERIS_ERROR(ceError =
				       CTCheckFileOrLinkExists("/usr/bin/sed",
							       &sedFound));
		if (sedFound)
			ppszArgs[argPos++] = "/usr/bin/sed";
		else
			BAIL_ON_CENTERIS_ERROR(ceError =
					       CENTERROR_SED_NOT_FOUND);
	}

	if (bDashN)
		ppszArgs[argPos++] = "-n";
	ppszArgs[argPos++] = pszExpression;

	BAIL_ON_CENTERIS_ERROR(ceError =
			       CTAllocateStringPrintf(&tempPath,
						      "%s.lwidentity.temp",
						      pszDstPath));

	dwFdIn = open(pszSrcPath, O_RDONLY);
	if (dwFdIn < 0) {
		ceError = CTMapSystemError(errno);
		BAIL_ON_CENTERIS_ERROR(ceError);
	}

	dwFdOut = open(tempPath, O_WRONLY | O_EXCL | O_CREAT, S_IWUSR);
	if (dwFdOut < 0) {
		ceError = CTMapSystemError(errno);
		BAIL_ON_CENTERIS_ERROR(ceError);
	}

	BAIL_ON_CENTERIS_ERROR(ceError =
			       CTGetOwnerAndPermissions(pszSrcPath, &uid, &gid,
							&mode));
	BAIL_ON_CENTERIS_ERROR(ceError =
			       CTChangeOwnerAndPermissions(tempPath, uid, gid,
							   mode));

	ceError =
	    CTSpawnProcessWithFds(ppszArgs[0], ppszArgs, dwFdIn, dwFdOut, 2,
				  &pProcInfo);
	BAIL_ON_CENTERIS_ERROR(ceError);

	ceError = CTGetExitStatus(pProcInfo, &status);
	BAIL_ON_CENTERIS_ERROR(ceError);

	if (status != 0) {
		BAIL_ON_CENTERIS_ERROR(ceError = CENTERROR_COMMAND_FAILED);
	}

	BAIL_ON_CENTERIS_ERROR(ceError =
			       CTFileContentsSame(tempPath, pszDstPath,
						  &isSame));
	if (isSame) {
		BAIL_ON_CENTERIS_ERROR(ceError = CTRemoveFile(tempPath));
	} else {
		BAIL_ON_CENTERIS_ERROR(ceError = CTBackupFile(pszDstPath));
		BAIL_ON_CENTERIS_ERROR(ceError =
				       CTMoveFile(tempPath, pszDstPath));
	}

      error:
	if (dwFdIn != -1)
		close(dwFdIn);
	if (dwFdOut != -1) {
		close(dwFdOut);
		if (!CENTERROR_IS_OK(ceError)) {
			CTRemoveFile(tempPath);
		}
	}
	if (pProcInfo)
		CTFreeProcInfo(pProcInfo);
	CT_SAFE_FREE_STRING(tempPath);

	return ceError;
}
