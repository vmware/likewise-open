
NTSTATUS
SmbProcessFindFirst2(
    PSMB_CONNECTION pSmbRequest
    );

NTSTATUS
SrvTrans2FindFirst2(
    HANDLE hTreeObject,
    USHORT SearchAttributes,
    USHORT Flags,
    USHORT InformationLevel,
    ULONG SearchStorageType,
    LPWSTR FileName,
    USHORT * pusSid,
    USHORT * puSearchCount,
    USHORT * pusEndofSearch,
    USHORT * pusLastNameOffset,
    PVOID * lppBuffer
    );

NTSTATUS
UnmarshallFindFirst2Request(
    PSMB_CONNECTION pSmbRequest
    );

NTSTATUS
MarshallFindFirst2Response(
    PSMB_CONNECTION pSmbRequest
    );

