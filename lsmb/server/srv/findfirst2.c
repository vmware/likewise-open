#include "includes.h"

NTSTATUS
SmbProcessFindFirst2(
    PSMB_CONNECTION pSmbRequest
    )
{
    NTSTATUS ntStatus = 0;
    HANDLE hTreeObject = (HANDLE)NULL;

    ntStatus = UnmarshallFindFirst2Request(pSmbRequest);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SrvTrans2FindFirst2(
                        hTreeObject,
                        0,
                        0,
                        0,
                        0,
                        NULL,
                        NULL,
                        NULL,
                        NULL,
                        NULL,
                        NULL);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = MarshallFindFirst2Response(pSmbRequest);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SmbSendReply(pSmbRequest);
    BAIL_ON_NT_STATUS(ntStatus);

error:

    return (ntStatus);
}

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
    )
{
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
UnmarshallFindFirst2Request(
    PSMB_CONNECTION pSmbRequest
    )
{
    NTSTATUS ntStatus = 0;

    return (ntStatus);
}


NTSTATUS
MarshallFindFirst2Response(
    PSMB_CONNECTION pSmbRequest
    )
{
    NTSTATUS ntStatus = 0;

    return (ntStatus);

}
