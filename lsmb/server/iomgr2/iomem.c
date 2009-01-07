#include "iop.h"

VOID
IoMemoryZero(
    IN OUT PVOID pMemory,
    IN size_t Size
    )
{
    memset(pMemory, 0, Size);
}

PVOID
IoMemoryAllocate(
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
IoMemoryFree(
    IN OUT PVOID pMemory
    )
{
    assert(pMemory);
    free(pMemory);
}

