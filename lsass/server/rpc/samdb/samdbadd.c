
#include "includes.h"

NTSTATUS
DirectoryAddObject(
    HANDLE hBindHandle,
    PWSTR ObjectDN,
    PDIRECTORY_MODS Attributes[]
    )
{
    NTSTATUS ntStatus = 0;
    PDIRECTORY_CONTEXT pDirectoryContext = hBindHandle;

   switch(pDirectoryContext->DirectoryType) {

        case LOCAL_SAM:
            ntStatus = LocalSamAddObject(
                            pDirectoryContext->hBindHandle,
                            ObjectDN,
                            Attributes
                            );
            break;

        default:
            break;
    }

    return ntStatus;
}
