#include "includes.h"

NET_API_STATUS
SrvSvcFreeMemory(
    void *pMemory
    )
{
    free(pMemory);
}

