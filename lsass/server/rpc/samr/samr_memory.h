#ifndef _SAMRSRV_MEMORY_H_
#define _SAMRSRV_MEMORY_H_


NTSTATUS
SamrSrvInitMemory(
    void
    );


NTSTATUS
SamrSrvDestroyMemory(
    void
    );


NTSTATUS
SamrSrvAllocateMemory(
    void **ppOut,
    DWORD dwSize,
    void *pDep
    );


void
SamrSrvFreeMemory(
    void *pPtr
    );


#endif /* _SAMRSRV_MEMORY_H_ */
