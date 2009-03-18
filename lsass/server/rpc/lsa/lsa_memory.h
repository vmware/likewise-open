#ifndef _LSASRV_MEMORY_H_
#define _LSASRV_MEMORY_H_


NTSTATUS
LsaSrvInitMemory(
    void
    );


NTSTATUS
LsaSrvDestroyMemory(
    void
    );


NTSTATUS
LsaSrvAllocateMemory(
    void **ppOut,
    DWORD dwSize,
    void *pDep
    );


void
LsaSrvFreeMemory(
    void *pPtr
    );


#endif /* _LSASRV_MEMORY_H_ */
