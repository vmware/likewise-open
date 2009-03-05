#include "includes.h"

NTSTATUS
DirectoryDeleteObject(
    HANDLE hBindHandle,
    PWSTR ObjectDN,
    )
{
    NTSTATUS ntStatus = 0;
    PDIRECTORY_CONTEXT pDirectoryContext = hBindHandle;

   switch(pDirectoryContext->DirectoryType) {

        case LOCAL_SAM:
            ntStatus = LocalSamDeleteObject(
                            pDirectoryContext->hBindHandle,
                            ObjectDN
                            );
            break;

        default:
            break;
    }

    return ntStatus;
}
