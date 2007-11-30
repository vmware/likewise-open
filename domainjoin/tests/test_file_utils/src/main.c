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

#define TEST_GET_MATCHING_FILE_PATHS_IN_FOLDER 1

#if TEST_GET_MATCHING_FILE_PATHS_IN_FOLDER
CENTERROR TestGetMatchingFilePathsInFolder()
{
	CENTERROR ceError = CENTERROR_SUCCESS;
	PSTR *ppszFilePaths = NULL;
	DWORD nPaths = 0;
	int iPath = 0;

	ceError = CTGetMatchingFilePathsInFolder("/etc",
						 "nss.*",
						 &ppszFilePaths, &nPaths);
	BAIL_ON_CENTERIS_ERROR(ceError);

	for (iPath = 0; iPath < nPaths; iPath++)
		printf("File path:%s\n", *(ppszFilePaths + iPath));

	if (nPaths == 0)
		printf("No paths were found\n");

      error:

	if (ppszFilePaths)
		CTFreeStringArray(ppszFilePaths, nPaths);

	return ceError;
}
#endif

int main(int argc, char *argv[])
{
	CENTERROR ceError = CENTERROR_SUCCESS;

#if TEST_CREATE_DIRECTORY
	ceError = DJCreateDirectory("/tmp/mydir", S_IRUSR);
	BAIL_ON_CENTERIS_ERROR(ceError);
#endif

#if TEST_GET_MATCHING_FILE_PATHS_IN_FOLDER
	ceError = TestGetMatchingFilePathsInFolder();
	BAIL_ON_CENTERIS_ERROR(ceError);
#endif

      error:

	return (ceError);
}
