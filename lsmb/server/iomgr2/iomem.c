#include "iop.h"

PVOID
IoAllocate(
    IN size_t Size
    )
{
    PVOID pointer = NULL;
    SMBAllocateMemory(Size, &pointer);
    return pointer;
}


VOID
IoFree(
    PVOID Pointer
    )
{
    SMBFreeMemory(Pointer);
}

NTSTATUS
IopDuplicateString(
    OUT PSTR* ppszNewString,
    IN PCSTR pszOriginalString
    )
{
    return SMBAllocateString(pszOriginalString, ppszNewString) ? STATUS_INSUFFICIENT_RESOURCES : STATUS_SUCCESS;
}
