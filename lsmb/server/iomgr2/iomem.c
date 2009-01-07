#include "iop.h"

PVOID
IoAllocate(
    IN size_t Size
    )
{
    PVOID pMemory = NULL;

    // TODO-Document behavior for Size == 0.
    assert(Size > 0);

    // Note -- If this allocator changes, need to change iostring routines.
    pMemory = malloc(Size);
    if (pMemory)
    {
        memset(pMemory, 0, Size);
    }

    return pMemory;
}


VOID
IoFree(
    PVOID pMemory
    )
{
    assert(pMemory);
    free(pMemory);
}
