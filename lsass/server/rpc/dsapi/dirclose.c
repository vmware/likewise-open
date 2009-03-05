#include "includes.h"

NTSTATUS
DirectoryClose(
    HANDLE hDirectory,
    )
{
    NTSTATUS ntStatus = 0;
    PDIRECTORY_CONTEXT pDirectoryContext = hBindHandle;

   switch(pDirectoryContext->DirectoryType) {

        case LOCAL_SAM:
            ntStatus = LocalSamClose(
                            pDirectoryContext->hBindHandle,
                            );
            break;

        default:
            break;
    }

    return ntStatus;
}
