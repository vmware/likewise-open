#include "includes.h"

NTSTATUS
WireUnmarshallFlushRequest(
    const PBYTE pParams,
    ULONG       ulBytesAvailable,
    ULONG       ulOffset,
    PFLUSH_REQUEST_HEADER* ppRequestHeader
    )
{
    NTSTATUS ntStatus = 0;
    PBYTE pDataCursor = pParams;
    PFLUSH_REQUEST_HEADER pRequestHeader = NULL;

    if (ulBytesAvailable < sizeof(FLUSH_REQUEST_HEADER))
    {
        ntStatus = STATUS_INVALID_BUFFER_SIZE;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    pRequestHeader = (PFLUSH_REQUEST_HEADER)pDataCursor;

    // pDataCursor += sizeof(FLUSH_REQUEST_HEADER);
    // ulOffset += sizeof(FLUSH_REQUEST_HEADER);

    *ppRequestHeader = pRequestHeader;

cleanup:

    return ntStatus;

error:

    *ppRequestHeader = NULL;

    goto cleanup;
}

NTSTATUS
WireMarshallFlushResponse(
    PBYTE   pParams,
    ULONG   ulBytesAvailable,
    ULONG   ulOffset,
    PFLUSH_RESPONSE_HEADER* ppResponseHeader,
    PUSHORT pusPackageBytesUsed
    )
{
    NTSTATUS ntStatus = 0;
    PFLUSH_RESPONSE_HEADER pResponseHeader = NULL;
    USHORT   usPackageBytesUsed = 0;

    if (ulBytesAvailable < sizeof(FLUSH_RESPONSE_HEADER))
    {
        ntStatus = STATUS_INVALID_BUFFER_SIZE;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    pResponseHeader = (PFLUSH_RESPONSE_HEADER)pParams;
    usPackageBytesUsed += sizeof(FLUSH_RESPONSE_HEADER);

    pResponseHeader->usByteCount = usPackageBytesUsed;

    *ppResponseHeader = pResponseHeader;
    *pusPackageBytesUsed = usPackageBytesUsed;

cleanup:

    return ntStatus;

error:

    *ppResponseHeader = NULL;
    *pusPackageBytesUsed = 0;

    goto cleanup;
}
