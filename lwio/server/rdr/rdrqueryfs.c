#include "rdr.h"

static
NTSTATUS
RdrCommonQueryVolumeInformation(
    PRDR_IRP_CONTEXT pIrpContext,
    PIRP pIrp
    );

NTSTATUS
RdrQueryVolumeInformation(
    IO_DEVICE_HANDLE IoDeviceHandle,
    PIRP pIrp
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;

    ntStatus = RdrCommonQueryVolumeInformation(
        NULL,
        pIrp
        );
    BAIL_ON_NT_STATUS(ntStatus);

error:

    return ntStatus;
}


static
NTSTATUS
RdrCommonQueryVolumeInformation(
    PRDR_IRP_CONTEXT pIrpContext,
    PIRP pIrp
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PSMB_CLIENT_FILE_HANDLE pFile = IoFileGetContext(pIrp->FileHandle);
    SMB_INFO_LEVEL infoLevel = 0;

    switch(pIrp->Args.QueryVolume.FsInformationClass)
    {
    case FileFsSizeInformation:
        infoLevel = SMB_INFO_ALLOCATION;
        break;
    default:
        ntStatus = STATUS_NOT_SUPPORTED;
        BAIL_ON_NT_STATUS(ntStatus);
        break;
    }

    ntStatus = RdrTransactQueryFsInfo(
        pFile->pTree,
        infoLevel,
        pIrp->Args.QueryVolume.FsInformation,
        pIrp->Args.QueryVolume.Length,
        &pIrp->IoStatusBlock.BytesTransferred);
    BAIL_ON_NT_STATUS(ntStatus);

error:

    pIrp->IoStatusBlock.Status = ntStatus;

    return ntStatus;
}
