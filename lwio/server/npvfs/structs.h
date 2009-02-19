#ifndef __NPFS_STRUCTS_H__
#define __NPFS_STRUCTS_H__

struct _NPFS_PIPE;
struct _NPFS_FCB;

typedef struct _NPFS_INTERLOCKED_ULONG
{
    ULONG           ulCounter;
    pthread_mutex_t CounterMutex;

} NPFS_INTERLOCKED_ULONG, *PNPFS_INTERLOCKED_ULONG;

typedef struct _NPFS_MDL
{
    ULONG Length;
    ULONG Offset;
    PVOID Buffer;

    struct _NPFS_MDL *pNext;

} NPFS_MDL, *PNPFS_MDL;

typedef struct _NPFS_CCB
{
    NPFS_INTERLOCKED_ULONG cRef;

    ULONG CcbType;

    ULONG CompletionState;
    ULONG ReadMode;

    struct _NPFS_PIPE * pPipe;
    PNPFS_MDL pMdlList;

    struct _NPFS_CCB * pNext;

} NPFS_CCB, *PNPFS_CCB;


typedef struct _NPFS_PIPE
{
    NPFS_INTERLOCKED_ULONG cRef;

    struct _NPFS_FCB *pFCB;
    pthread_mutex_t PipeMutex;
    pthread_cond_t PipeCondition;
    ULONG PipeClientState;
    ULONG PipeServerState;
    PNPFS_CCB pSCB;
    PNPFS_CCB pCCB;

    struct _NPFS_PIPE *pNext;

} NPFS_PIPE, *PNPFS_PIPE;

typedef struct _NPFS_FCB
{
    NPFS_INTERLOCKED_ULONG cRef;

    pthread_rwlock_t       PipeListRWLock;
    PNPFS_PIPE             pPipeList;
    PSECURITY_DESCRIPTOR   pSecurityDescriptor;
    UNICODE_STRING         PipeName;
    ULONG                  NamedPipeConfiguration;
    FILE_PIPE_TYPE_MASK    NamedPipeType;
    ULONG                  MaxNumberOfInstances;
    ULONG                  CurrentNumberOfInstances;
    ULONG                  Max;
    PNPFS_PIPE             pPipes;

    struct _NPFS_FCB *pNext;

}NPFS_FCB, *PNPFS_FCB;

typedef struct _NPFS_IRP_CONTEXT
{
    PIRP pIrp;
    IO_DEVICE_HANDLE TargetDeviceHandle;
    UNICODE_STRING RootPathName;
    UNICODE_STRING RelativePathName;
    UNICODE_STRING AbsolutePathName;

} NPFS_IRP_CONTEXT, *PNPFS_IRP_CONTEXT;

typedef enum _PVFS_INFO_TYPE
{
    NPFS_QUERY = 1,
    NPFS_SET
} NPFS_INFO_TYPE, *PNPFS_INFO_TYPE;

#endif /* __NPFS_STRUCTS_H__ */
