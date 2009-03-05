#include "includes.h"

NTSTATUS
DirectorySearch(
    HANDLE hDirectory,
    PWSTR Base,
    ULONG Scope,
    PWSTR Filter,
    PWSTR Attributes[],
    ULONG AttributesOnly,
    PDIRECTORY_VALUES * ppDirectoryValues
    PDWORD pdwNumValues
    )
{
    NTSTATUS ntStatus = 0;
    PDIRECTORY_CONTEXT pDirectoryContext = hBindHandle;

   switch(pDirectoryContext->DirectoryType) {

        case LOCAL_SAM:
            ntStatus = LocalSamBind(
                            pDirectoryContext->hBindHandle,
                            Base,
                            Scope,
                            Filter,
                            Attributes,
                            AttributesOnly,
                            ppDirectoryValues,
                            pdwNumValues
                            );
            break;

        default:
            break;
    }

    return ntStatus;
}
