#include "includes.h"

VOID
SrvSvcFreeMemory(
    PVOID pMemory
    )
{
    free(pMemory);
}

