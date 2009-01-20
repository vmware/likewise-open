#include "includes.h"

NTSTATUS
SrvGssCreate(
    PSRV_GSS_CONTEXT* ppGssContext
    )
{
    NTSTATUS ntStatus = 0;
    PSRV_GSS_CONTEXT pGssContext = NULL;

    ntStatus = SMBAllocateMemory(
                    sizeof(SRV_GSS_CONTEXT),
                    (PVOID*)&pGssContext);
    BAIL_ON_NT_STATUS(ntStatus);

    *ppGssContext = pGssContext;

cleanup:

    return ntStatus;

error:

    *ppGssContext = NULL;

    goto cleanup;
}

NTSTATUS
SrvGssNegotiate(
    PSRV_GSS_CONTEXT pGssContext,
    PBYTE            pSessionKey,
    ULONG            ulSessionKeyLength
    )
{
    NTSTATUS ntStatus = 0;

    return ntStatus;
}

VOID
SrvGssFree(
    PSRV_GSS_CONTEXT pGssContext
    )
{
    SMBFreeMemory(pGssContext);
}
