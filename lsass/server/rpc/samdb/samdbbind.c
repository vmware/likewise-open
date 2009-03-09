#include "includes.h"

NTSTATUS
SamDBBind(
    HANDLE hDirectory,
    PWSTR  DistinguishedName,
    PWSTR  Credential,
    ULONG  ulMethod
    )
{
    NTSTATUS ntStatus = 0;
    PDIRECTORY_CONTEXT pDirectoryContext = hBindHandle;

    return ntStatus;
}
