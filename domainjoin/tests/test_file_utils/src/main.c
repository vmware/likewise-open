/* Editor Settings: expandtabs and use 4 spaces for indentation
* ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
* -*- mode: c, c-basic-offset: 4 -*- */

#include "domainjoin.h"

#define TEST_GET_MATCHING_FILE_PATHS_IN_FOLDER 1

#if TEST_GET_MATCHING_FILE_PATHS_IN_FOLDER
CENTERROR
TestGetMatchingFilePathsInFolder()
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    PSTR* ppszFilePaths = NULL;
    DWORD nPaths = 0;
    int   iPath = 0;

    ceError = CTGetMatchingFilePathsInFolder("/etc",
                                             "nss.*",
                                             &ppszFilePaths,
                                             &nPaths);
    BAIL_ON_CENTERIS_ERROR(ceError);

    for (iPath = 0; iPath < nPaths; iPath++)
        printf("File path:%s\n", *(ppszFilePaths+iPath));

    if (nPaths == 0)
       printf("No paths were found\n");

error:

    if (ppszFilePaths)
       CTFreeStringArray(ppszFilePaths, nPaths);

    return ceError;
}
#endif

int
main(int argc, char* argv[])
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

    return(ceError);
}
