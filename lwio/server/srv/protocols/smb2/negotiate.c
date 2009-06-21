#include "includes.h"

NTSTATUS
SrvBuildNegotiateResponse_SMB_V2(
	IN  PLWIO_SRV_CONNECTION pConnection,
	IN  PSMB_PACKET          pSmbRequest,
	OUT PSMB_PACKET*         ppSmbResponse
	)
{
	NTSTATUS ntStatus = STATUS_SUCCESS;
	PSMB_PACKET pSmbResponse = NULL;
	PSMB2_NEGOTIATE_HEADER pNegotiateHeader = NULL;
	ULONG  ulBytesUsed = 0;
	ULONG  ulBytesAvailable = 0;
	ULONG  ulOffset = 0;
	USHORT usAlign = 0;
	PBYTE  pDataCursor = NULL;
	PBYTE  pSessionKey = NULL;
	ULONG  ulSessionKeyLength = 0;
	LONG64 llCurTime = 0LL;
	time_t curTime = 0;
	PSRV_PROPERTIES pServerProperties = &pConnection->serverProperties;

	ntStatus = SMBPacketAllocate(
					pConnection->hPacketAllocator,
					&pSmbResponse);
	BAIL_ON_NT_STATUS(ntStatus);

	ntStatus = SMBPacketBufferAllocate(
					pConnection->hPacketAllocator,
					64 * 1024,
					&pSmbResponse->pRawBuffer,
					&pSmbResponse->bufferLen);
	BAIL_ON_NT_STATUS(ntStatus);

	ntStatus = SMB2MarshalHeader(
				pSmbResponse,
				COM2_NEGOTIATE,
				1, /* usCredits    */
				0, /* usPid        */
				0, /* usTid        */
				0, /* ullSessionId */
				0, /* status       */
				TRUE);
	BAIL_ON_NT_STATUS(ntStatus);

	ulBytesAvailable = pSmbResponse->bufferLen - pSmbResponse->bufferUsed;

	if (ulBytesAvailable < sizeof(SMB2_NEGOTIATE_HEADER))
	{
		ntStatus = STATUS_INVALID_BUFFER_SIZE;
		BAIL_ON_NT_STATUS(ntStatus);
	}

	pNegotiateHeader = (PSMB2_NEGOTIATE_HEADER)(pSmbResponse->pParams);

	ulOffset = (PBYTE)pSmbResponse->pParams - (PBYTE)pSmbResponse->pSMB2Header;

	ulBytesUsed += sizeof(SMB2_NEGOTIATE_HEADER);
	ulBytesAvailable -= sizeof(SMB2_NEGOTIATE_HEADER);
	ulOffset += sizeof(SMB2_NEGOTIATE_HEADER);

	pNegotiateHeader->usDialect = 0x0202;

	pNegotiateHeader->ucFlags = 0;

	if (pServerProperties->bEnableSecuritySignatures)
	{
		pNegotiateHeader->ucFlags |= 0x1;
	}
	if (pServerProperties->bRequireSecuritySignatures)
	{
		pNegotiateHeader->ucFlags |= 0x2;
	}

	pNegotiateHeader->ulMaxReadSize = pServerProperties->MaxBufferSize;
	pNegotiateHeader->ulMaxWriteSize = pServerProperties->MaxBufferSize;
	pNegotiateHeader->ulCapabilities = 0;
	pNegotiateHeader->ulMaxTxSize = pServerProperties->MaxBufferSize;

	curTime = time(NULL);

	llCurTime = (curTime + 11644473600LL) * 10000000LL;

	pNegotiateHeader->ullCurrentTime = llCurTime;
	// TODO: Figure out boot time
	pNegotiateHeader->ullBootTime = llCurTime;

	memcpy(&pNegotiateHeader->serverGUID[0],
			pServerProperties->GUID,
			sizeof(pServerProperties->GUID));

	ntStatus = SrvGssBeginNegotiate(
					pConnection->hGssContext,
					&pConnection->hGssNegotiate);
	BAIL_ON_NT_STATUS(ntStatus);

	ntStatus = SrvGssNegotiate(
					pConnection->hGssContext,
					pConnection->hGssNegotiate,
					NULL,
					0,
					&pSessionKey,
					&ulSessionKeyLength);
	BAIL_ON_NT_STATUS(ntStatus);

	if (!ulSessionKeyLength)
	{
		ntStatus = STATUS_NOT_SUPPORTED;
		BAIL_ON_NT_STATUS(ntStatus);
	}

	usAlign = ulOffset % 8;
	if (ulBytesAvailable < usAlign)
	{
		ntStatus = STATUS_INVALID_BUFFER_SIZE;
		BAIL_ON_NT_STATUS(ntStatus);
	}

	ulOffset += usAlign;
	ulBytesUsed += usAlign;
	ulBytesAvailable -= usAlign;

	pNegotiateHeader->usBlobOffset = ulOffset;
	pNegotiateHeader->usBlobLength = ulSessionKeyLength;

	if (ulBytesAvailable < ulSessionKeyLength)
	{
		ntStatus = STATUS_INVALID_BUFFER_SIZE;
		BAIL_ON_NT_STATUS(ntStatus);
	}

	pDataCursor = (PBYTE)pSmbResponse->pSMB2Header + ulOffset;

	memcpy(pDataCursor, pSessionKey, ulSessionKeyLength);

	pNegotiateHeader->usLength = ulBytesUsed + 1; /* add one for dynamic part */

	ulBytesUsed += ulSessionKeyLength;

	pSmbResponse->bufferUsed += ulBytesUsed;

	ntStatus = SMB2PacketMarshallFooter(pSmbResponse);
	BAIL_ON_NT_STATUS(ntStatus);

	*ppSmbResponse = pSmbResponse;

cleanup:

	if (pSessionKey)
	{
		SrvFreeMemory(pSessionKey);
	}

	return ntStatus;

error:

	*ppSmbResponse = NULL;

	if (pSmbResponse)
	{
		SMBPacketFree(pConnection->hPacketAllocator, pSmbResponse);
	}

	goto cleanup;
}
