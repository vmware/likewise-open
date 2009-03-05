#include "includes.h"

NTSTATUS
DirBind(
    HANDLE hDirectory,
    PWSTR  DistinguishedName,
    PWSTR  Credential,
    ULONG  ulMethod
    )
{
    NTSTATUS ntStatus = 0;
    PDIRECTORY_CONTEXT pDirectoryContext = hBindHandle;

   switch(pDirectoryContext->DirectoryType) {

        case LOCAL_SAM:
            ntStatus = LocalSamBind(
                            pDirectoryContext->hBindHandle,
                            DistinguishedName,
                            Credential,
                            ulMethod
                            );
            break;

        default:
            break;
    }

    return ntStatus;
}
