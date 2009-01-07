
NTSTATUS
SmbProcessLockAndX(
    PSMB_CONNECTION pSmbRequest
    );

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
    );

NTSTATUS
UnmarshallLockAndXRequest(
    PSMB_CONNECTION pSmbRequest
    );

NTSTATUS
MarshallLockAndXResponse(
    PSMB_CONNECTION pSmbRequest
    );

