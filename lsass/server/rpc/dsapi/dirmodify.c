#include "includes.h"

NTSTATUS
DirectoryModifyObject(
    HANDLE hBindHandle,
    PWSTR ObjectDN,
    PDIRECTORY_MODS Modifications[]
    )
{
    NTSTATUS ntStatus = 0;
    PDIRECTORY_CONTEXT pDirectoryContext = hBindHandle;

   switch(pDirectoryContext->DirectoryType) {

        case LOCAL_SAM:
            ntStatus = LocalSamModifyObject(
                            pDirectoryContext->hBindHandle,
                            ObjectDN,
                            Modifications
                            );
            break;

        default:
            break;
    }

    return ntStatus;
}
