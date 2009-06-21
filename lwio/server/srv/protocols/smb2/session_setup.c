
#include "includes.h"

NTSTATUS
SrvProcessSessionSetup_SMB_V2(
	PLWIO_SRV_CONNECTION pConnection,
	PSMB_PACKET          pSmbRequest,
	PSMB_PACKET*         ppSmbResponse
	)
{
	NTSTATUS ntStatus = STATUS_SUCCESS;
	PSMB_PACKET pSmbResponse = NULL;

	ntStatus = STATUS_NOT_IMPLEMENTED;
	BAIL_ON_NT_STATUS(ntStatus);

	*ppSmbResponse = pSmbResponse;

cleanup:

	return ntStatus;

error:

	*ppSmbResponse = NULL;

	if (pSmbResponse)
	{
		SMBPacketFree(pConnection->hPacketAllocator, pSmbResponse);
	}

	goto cleanup;
}
