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
#ifndef __CTFILEUTILS_H__
#define __CTFILEUTILS_H__

CENTERROR CTRemoveFile(PCSTR pszPath);

CENTERROR CTRemoveDirectory(PCSTR pszPath);

CENTERROR CTCheckFileExists(PCSTR pszPath, PBOOLEAN pbFileExists);

CENTERROR CTCheckLinkExists(PCSTR pszPath, PBOOLEAN pbLinkExists);

CENTERROR CTCheckFileOrLinkExists(PCSTR pszPath, PBOOLEAN pbExists);

CENTERROR CTGetSymLinkTarget(PCSTR pszPath, PSTR * ppszTargetPath);

CENTERROR CTCheckDirectoryExists(PCSTR pszPath, PBOOLEAN pbFileExists);

CENTERROR CTCheckSockExists(PCSTR pszPath, PBOOLEAN pbSockExists);

CENTERROR CTCreateSymLink(PCSTR pszOldPath, PCSTR pszNewPath);

CENTERROR CTMoveFile(PCSTR pszSrcPath, PCSTR pszDstPath);

CENTERROR
CTMoveFileEx(PCSTR pszSrcPath, PCSTR pszDstPath, BOOLEAN bCrossDevice);

CENTERROR
CTCopyFileWithPerms(PCSTR pszSrcPath, PCSTR pszDstPath, mode_t dwPerms);

CENTERROR CTCopyFileWithOriginalPerms(PCSTR pszSrcPath, PCSTR pszDstPath);

CENTERROR CTChangePermissions(PCSTR pszPath, mode_t dwFileMode);

CENTERROR
CTGetOwnerAndPermissions(PCSTR pszSrcPath,
			 uid_t * uid, gid_t * gid, mode_t * mode);

CENTERROR
CTGetFileTimeStamps(PCSTR pszFilePath,
		    time_t * patime, time_t * pmtime, time_t * pctime);

CENTERROR CTChangeOwner(PCSTR pszPath, uid_t uid, gid_t gid);

CENTERROR
CTChangeOwnerAndPermissions(PCSTR pszPath,
			    uid_t uid, gid_t gid, mode_t dwFileMode);

CENTERROR CTCreateDirectory(PSTR pszPath, mode_t dwFileMode);

CENTERROR
CTGetMatchingFilePathsInFolder(PSTR pszDirPath,
			       PSTR pszFileNameRegExp,
			       PSTR ** pppszHostFilePaths, PDWORD pdwNPaths);

CENTERROR
CTGetMatchingDirPathsInFolder(PSTR pszDirPath,
			      PSTR pszDirNameRegExp,
			      PSTR ** pppszHostDirPaths, PDWORD pdwNPaths);

CENTERROR
CTCheckFileHoldsPattern(PCSTR pszFilePath,
			PCSTR pszPattern, PBOOLEAN pbPatternExists);

CENTERROR
CTFileContentsSame(PCSTR pszFilePath1, PCSTR pszFilePath2, PBOOLEAN pbSame);

CENTERROR CTGetAbsolutePath(PSTR pszRelativePath, PSTR * ppszAbsolutePath);

CENTERROR CTRemoveFiles(PSTR pszPath, BOOLEAN fDirectory, BOOLEAN fRecursive);

CENTERROR CTBackupFile(PCSTR path);

CENTERROR CTReadFile(PCSTR pszFilePath, PSTR * ppBuffer, PLONG pSize);

CENTERROR CTOpenFile(PCSTR path, PCSTR mode, FILE ** handle);

CENTERROR CTFileStreamWrite(FILE * handle, PCSTR data, unsigned int size);

CENTERROR CTFilePrintf(FILE * handle, PCSTR format, ...
    );

CENTERROR CTCloseFile(FILE * handle);

CENTERROR CTMoveFileAcrossDevices(PCSTR pszSrcPath, PCSTR pszDstPath);

CENTERROR CTGetCurrentDirectoryPath(PSTR * ppszPath);

/** This will run a sed expression on the src file and save the ouput into the dst file. It is safe to use the same path for the src and dst file names.

The dst file will be backed up before it is changed. After the sed operation finishes, the dst file will have the permissions of the src file.

The worst that can happen if this command is used to write to a user writeable directory, is that the function can fail if a user has already created the temporary file that this function uses.

So, that means if this function succeeds, a nonroot user did not tampered with the output file before or during the sed operation.
*/
CENTERROR
CTRunSedOnFile(PCSTR pszSrcPath,
	       PCSTR pszDstPath, BOOLEAN bDashN, PCSTR pszExpression);

#endif				/* __CTFILEUTILS_H__ */
