#include "includes.h"

VOID
SamDbClose(
    HANDLE hDirectory
    )
{
    PSAM_DIRECTORY_CONTEXT pDirContext = (PSAM_DIRECTORY_CONTEXT)hDirectory;

    if (pDirContext)
    {
        SamDbFreeDirectoryContext(pDirContext);
    }
}
