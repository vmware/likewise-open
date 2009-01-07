#include "includes.h"

NTSTATUS
SmbProcessLockAndX(
    PSMB_CONNECTION pSmbRequest
    )
{
    NTSTATUS ntStatus = 0;
    HANDLE hTreeObject = (HANDLE)NULL;
    LOCKING_ANDX_RANGE locks[] = {0};
    LOCKING_ANDX_RANGE unlocks[] = {0};

    ntStatus = UnmarshallLockAndXRequest(pSmbRequest);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SrvLockFile(
                        hTreeObject,
                        0,
                        'C',
                        0,
                        0,
                        0,
                        unlocks,
                        locks);
    BAIL_ON_NT_STATUS(ntStatus);


    ntStatus = MarshallLockAndXResponse(pSmbRequest);
    BAIL_ON_NT_STATUS(ntStatus);


    ntStatus = SmbSendReply(pSmbRequest);
    BAIL_ON_NT_STATUS(ntStatus);

error:

    return (ntStatus);
}

NTSTATUS
SrvLockFile(
    HANDLE hTreeObject,
    USHORT usFid,
    UCHAR LockType,
    ULONG TimeOut,
    USHORT NumberofUnlocks,
    USHORT NumberOfLocks,
    LOCKING_ANDX_RANGE Unlocks[],
    LOCKING_ANDX_RANGE Locks[]
    )
{
    return STATUS_NOT_IMPLEMENTED;
}


NTSTATUS
UnmarshallLockAndXRequest(
    PSMB_CONNECTION pSmbRequest
    )
{
    NTSTATUS ntStatus = 0;

    return (ntStatus);
}


NTSTATUS
MarshallLockAndXResponse(
    PSMB_CONNECTION pSmbRequest
    )
{
    NTSTATUS ntStatus = 0;

    return (ntStatus);

}
