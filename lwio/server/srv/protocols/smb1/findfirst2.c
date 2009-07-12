/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

/*
 * Copyright Likewise Software
 * All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or (at
 * your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * for more details.  You should have received a copy of the GNU General
 * Public License along with this program.  If not, see
 * <http://www.gnu.org/licenses/>.
 *
 * LIKEWISE SOFTWARE MAKES THIS SOFTWARE AVAILABLE UNDER OTHER LICENSING
 * TERMS AS WELL.  IF YOU HAVE ENTERED INTO A SEPARATE LICENSE AGREEMENT
 * WITH LIKEWISE SOFTWARE, THEN YOU MAY ELECT TO USE THE SOFTWARE UNDER THE
 * TERMS OF THAT SOFTWARE LICENSE AGREEMENT INSTEAD OF THE TERMS OF THE GNU
 * GENERAL PUBLIC LICENSE, NOTWITHSTANDING THE ABOVE NOTICE.  IF YOU
 * HAVE QUESTIONS, OR WISH TO REQUEST A COPY OF THE ALTERNATE LICENSING
 * TERMS OFFERED BY LIKEWISE SOFTWARE, PLEASE CONTACT LIKEWISE SOFTWARE AT
 * license@likewisesoftware.com
 */

#include "includes.h"

static
NTSTATUS
SrvUnmarshallFindFirst2Params(
    PBYTE           pParams,
    USHORT          usBytesAvailable,
    USHORT          usParameterOffset,
    PUSHORT         pusSearchAttrs,
    PUSHORT         pusSearchCount,
    PUSHORT         pusFlags,
    PSMB_INFO_LEVEL pSmbInfoLevel,
    PULONG          pulSearchStorageType,
    PWSTR*          ppwszSearchPattern
    );

static
NTSTATUS
SrvBuildFindFirst2Response(
    PLWIO_SRV_CONNECTION pConnection,
    PSMB_PACKET         pSmbRequest,
    USHORT              usSearchAttrs,
    USHORT              usSearchCount,
    USHORT              usFlags,
    SMB_INFO_LEVEL      infoLevel,
    ULONG               ulSearchStorageType,
    PWSTR               pwszSearchPattern,
    USHORT              usMaxDataCount,
    PSMB_PACKET*        ppSmbResponse
    );

NTSTATUS
SrvProcessTrans2FindFirst2(
    IN  PLWIO_SRV_CONNECTION        pConnection,
    IN  PSMB_PACKET                 pSmbRequest,
    IN  PTRANSACTION_REQUEST_HEADER pRequestHeader,
    IN  PUSHORT                     pSetup,
    IN  PUSHORT                     pByteCount,
    IN  PBYTE                       pParameters,
    IN  PBYTE                       pData,
    OUT PSMB_PACKET*                ppSmbResponse
    )
{
    NTSTATUS ntStatus = 0;
    USHORT         usSearchAttrs = 0;
    USHORT         usSearchCount = 0;
    USHORT         usFlags = 0;
    SMB_INFO_LEVEL infoLevel = 0;
    ULONG          ulSearchStorageType = 0;
    PWSTR          pwszSearchPattern = NULL; // Do not free
    PSMB_PACKET    pSmbResponse = NULL;

    ntStatus = SrvUnmarshallFindFirst2Params(
                    pParameters,
                    pRequestHeader->parameterCount,
                    pRequestHeader->parameterOffset,
                    &usSearchAttrs,
                    &usSearchCount,
                    &usFlags,
                    &infoLevel,
                    &ulSearchStorageType,
                    &pwszSearchPattern);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SrvBuildFindFirst2Response(
                    pConnection,
                    pSmbRequest,
                    usSearchAttrs,
                    usSearchCount,
                    usFlags,
                    infoLevel,
                    ulSearchStorageType,
                    pwszSearchPattern,
                    pRequestHeader->maxDataCount,
                    &pSmbResponse);
    BAIL_ON_NT_STATUS(ntStatus);

    *ppSmbResponse = pSmbResponse;

cleanup:

    return ntStatus;

error:

    *ppSmbResponse = NULL;

    if (pSmbResponse)
    {
        SMBPacketFree(
            pConnection->hPacketAllocator,
            pSmbResponse);
    }

    goto cleanup;
}

static
NTSTATUS
SrvUnmarshallFindFirst2Params(
    PBYTE           pParams,
    USHORT          usBytesAvailable,
    USHORT          usParameterOffset,
    PUSHORT         pusSearchAttrs,
    PUSHORT         pusSearchCount,
    PUSHORT         pusFlags,
    PSMB_INFO_LEVEL pInfoLevel,
    PULONG          pulSearchStorageType,
    PWSTR*          ppwszSearchPattern
    )
{
    NTSTATUS ntStatus = 0;
    PBYTE    pDataCursor = pParams;
    USHORT   usSearchAttrs = 0;
    USHORT   usSearchCount = 0;
    USHORT   usFlags = 0;
    SMB_INFO_LEVEL infoLevel = 0;
    ULONG    ulSearchStorageType = 0;
    PWSTR    pwszSearchPattern = NULL;
    USHORT   usAlignment = 0;

    // TODO: Is this necessary?
    //
    usAlignment = usParameterOffset % 2;

    if (usBytesAvailable < usAlignment)
    {
        ntStatus = STATUS_DATA_ERROR;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    pDataCursor += usAlignment;
    usParameterOffset += usAlignment;
    usBytesAvailable -= usAlignment;

    if (usBytesAvailable < sizeof(usSearchAttrs))
    {
        ntStatus = STATUS_DATA_ERROR;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    usSearchAttrs = *((PUSHORT)pDataCursor);
    pDataCursor += sizeof(usSearchAttrs);
    usParameterOffset += sizeof(usSearchAttrs);
    usBytesAvailable -= sizeof(usSearchAttrs);

    if (usBytesAvailable < sizeof(usSearchCount))
    {
        ntStatus = STATUS_DATA_ERROR;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    usSearchCount = *((PUSHORT)pDataCursor);
    pDataCursor += sizeof(usSearchCount);
    usParameterOffset += sizeof(usSearchCount);
    usBytesAvailable -= sizeof(usSearchCount);

    if (usBytesAvailable < sizeof(usFlags))
    {
        ntStatus = STATUS_DATA_ERROR;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    usFlags = *((PUSHORT)pDataCursor);
    pDataCursor += sizeof(usFlags);
    usParameterOffset += sizeof(usFlags);
    usBytesAvailable -= sizeof(usFlags);

    if (usBytesAvailable < sizeof(infoLevel))
    {
        ntStatus = STATUS_DATA_ERROR;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    infoLevel = *((PSMB_INFO_LEVEL)pDataCursor);
    pDataCursor += sizeof(SMB_INFO_LEVEL);
    usParameterOffset += sizeof(SMB_INFO_LEVEL);
    usBytesAvailable -= sizeof(SMB_INFO_LEVEL);

    if (usBytesAvailable < sizeof(ulSearchStorageType))
    {
        ntStatus = STATUS_DATA_ERROR;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    ulSearchStorageType = *((PULONG)pDataCursor);
    pDataCursor += sizeof(ulSearchStorageType);
    usParameterOffset += sizeof(ulSearchStorageType);
    usBytesAvailable -= sizeof(ulSearchStorageType);

    if (usBytesAvailable)
    {
        pwszSearchPattern = (PWSTR)pDataCursor;
    }

    *pusSearchAttrs = usSearchAttrs;
    *pusSearchCount = usSearchCount;
    *pusFlags = usFlags;
    *pInfoLevel = infoLevel;
    *pulSearchStorageType = ulSearchStorageType;
    *ppwszSearchPattern = pwszSearchPattern;

cleanup:

    return ntStatus;

error:

    *pusSearchAttrs = 0;
    *pusSearchCount = 0;
    *pusFlags = 0;
    *pInfoLevel = 0;
    *pulSearchStorageType = 0;
    *ppwszSearchPattern = NULL;

    goto cleanup;
}

static
NTSTATUS
SrvBuildFindFirst2Response(
    PLWIO_SRV_CONNECTION pConnection,
    PSMB_PACKET         pSmbRequest,
    USHORT              usSearchAttrs,
    USHORT              usSearchCount,
    USHORT              usFlags,
    SMB_INFO_LEVEL      infoLevel,
    ULONG               ulSearchStorageType,
    PWSTR               pwszSearchPattern,
    USHORT              usMaxDataCount,
    PSMB_PACKET*        ppSmbResponse
    )
{
    NTSTATUS ntStatus = 0;
    PLWIO_SRV_SESSION pSession = NULL;
    PLWIO_SRV_TREE pTree = NULL;
    SMB_FIND_FIRST2_RESPONSE_PARAMETERS responseParams = {0};
    BOOLEAN   bReturnSingleEntry = FALSE;
    BOOLEAN   bRestartScan = FALSE;
    wchar16_t wszBackSlash[2] = {0, 0};
    USHORT    usDataOffset = 0;
    USHORT    usParameterOffset = 0;
    USHORT    usNumPackageBytesUsed = 0;
    ULONG     usBytesAvailable = 0;
    USHORT    usSearchResultLen = 0;
    USHORT    usSearchId = 0;
    PBYTE     pData = NULL;
    HANDLE    hSearchSpace = NULL;
    PUSHORT   pSetup = NULL;
    BYTE      setupCount = 0;
    BOOLEAN   bEndOfSearch = FALSE;
    BOOLEAN   bInLock = FALSE;
    PWSTR     pwszFilesystemPath = NULL;
    PWSTR     pwszSearchPattern2 = NULL;
    PSMB_PACKET pSmbResponse = NULL;

    ntStatus = SrvConnectionFindSession(
                    pConnection,
                    pSmbRequest->pSMBHeader->uid,
                    &pSession);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SrvSessionFindTree(
                    pSession,
                    pSmbRequest->pSMBHeader->tid,
                    &pTree);
    BAIL_ON_NT_STATUS(ntStatus);

    wcstowc16s(&wszBackSlash[0], L"\\", 1);

    if (pwszSearchPattern && *pwszSearchPattern == wszBackSlash[0])
        pwszSearchPattern++;

    LWIO_LOCK_RWMUTEX_SHARED(bInLock, &pTree->pShareInfo->mutex);

    ntStatus = SrvBuildSearchPath(
                    pTree->pShareInfo->pwszPath,
                    pwszSearchPattern,
                    &pwszFilesystemPath,
                    &pwszSearchPattern2);
    BAIL_ON_NT_STATUS(ntStatus);

    LWIO_UNLOCK_RWMUTEX(bInLock, &pTree->pShareInfo->mutex);

    ntStatus = SrvFinderCreateSearchSpace(
                    pSession->pIoSecurityContext,
                    pSession->hFinderRepository,
                    pwszFilesystemPath,
                    pwszSearchPattern2,
                    usSearchAttrs,
                    ulSearchStorageType,
                    infoLevel,
                    (pSmbRequest->pSMBHeader->flags2 & FLAG2_KNOWS_LONG_NAMES ? TRUE : FALSE),
                    &hSearchSpace,
                    &usSearchId);
    BAIL_ON_NT_STATUS(ntStatus);

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

    ntStatus = SMBPacketMarshallHeader(
                pSmbResponse->pRawBuffer,
                pSmbResponse->bufferLen,
                COM_TRANSACTION2,
                0,
                TRUE,
                pSmbRequest->pSMBHeader->tid,
                pSmbRequest->pSMBHeader->pid,
                pSmbRequest->pSMBHeader->uid,
                pSmbRequest->pSMBHeader->mid,
                pConnection->serverProperties.bRequireSecuritySignatures,
                pSmbResponse);
    BAIL_ON_NT_STATUS(ntStatus);

    pSmbResponse->pSMBHeader->wordCount = 10 + setupCount;

    ntStatus = WireMarshallTransaction2Response(
                    pSmbResponse->pParams,
                    pSmbResponse->bufferLen - pSmbResponse->bufferUsed,
                    (PBYTE)pSmbResponse->pParams - (PBYTE)pSmbResponse->pSMBHeader,
                    pSetup,
                    setupCount,
                    (PBYTE)&responseParams,
                    sizeof(responseParams),
                    NULL,
                    0,
                    &usDataOffset,
                    &usParameterOffset,
                    &usNumPackageBytesUsed);
    BAIL_ON_NT_STATUS(ntStatus);

    usBytesAvailable = SMB_MIN(usMaxDataCount, pConnection->serverProperties.MaxBufferSize - usNumPackageBytesUsed);

    ntStatus = SrvFinderGetSearchResults(
                    hSearchSpace,
                    bReturnSingleEntry,
                    bRestartScan,
                    usSearchCount,
                    usBytesAvailable,
                    usDataOffset,
                    &pData,
                    &usSearchResultLen,
                    &responseParams.usSearchCount,
                    &bEndOfSearch);
    BAIL_ON_NT_STATUS(ntStatus);

    responseParams.usSearchId = usSearchId;

    if (bEndOfSearch || (usFlags & SMB_FIND_CLOSE_AFTER_REQUEST))
    {
        responseParams.usEndOfSearch = 1;
    }

    ntStatus = WireMarshallTransaction2Response(
                    pSmbResponse->pParams,
                    pSmbResponse->bufferLen - pSmbResponse->bufferUsed,
                    (PBYTE)pSmbResponse->pParams - (PBYTE)pSmbResponse->pSMBHeader,
                    pSetup,
                    setupCount,
                    (PBYTE)&responseParams,
                    sizeof(responseParams),
                    pData,
                    usSearchResultLen,
                    &usDataOffset,
                    &usParameterOffset,
                    &usNumPackageBytesUsed);
    BAIL_ON_NT_STATUS(ntStatus);

    pSmbResponse->bufferUsed += usNumPackageBytesUsed;

    ntStatus = SMBPacketUpdateAndXOffset(pSmbResponse);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SMBPacketMarshallFooter(pSmbResponse);
    BAIL_ON_NT_STATUS(ntStatus);

    *ppSmbResponse = pSmbResponse;

cleanup:

    if (hSearchSpace)
    {
        SrvFinderReleaseSearchSpace(hSearchSpace);

        if ((usFlags & SMB_FIND_CLOSE_AFTER_REQUEST) ||
            (bEndOfSearch && (usFlags & SMB_FIND_CLOSE_IF_EOS)))
        {
            SrvFinderCloseSearchSpace(
                pSession->hFinderRepository,
                usSearchId);
        }
    }

    if (pTree)
    {
        LWIO_UNLOCK_RWMUTEX(bInLock, &pTree->pShareInfo->mutex);

        SrvTreeRelease(pTree);
    }

    if (pSession)
    {
        SrvSessionRelease(pSession);
    }

    if (pData)
    {
        SrvFreeMemory(pData);
    }
    if (pwszFilesystemPath)
    {
        SrvFreeMemory(pwszFilesystemPath);
    }
    if (pwszSearchPattern2)
    {
        SrvFreeMemory(pwszSearchPattern2);
    }

    return ntStatus;

error:

    *ppSmbResponse = NULL;

    if (pSmbResponse)
    {
        SMBPacketFree(
            pConnection->hPacketAllocator,
            pSmbResponse);
    }

    if (ntStatus == STATUS_NO_MORE_MATCHES)
    {
        ntStatus = STATUS_NO_SUCH_FILE;
    }

    goto cleanup;
}

NTSTATUS
SrvBuildSearchPath(
    IN  PWSTR  pwszPath,
    IN  PWSTR  pwszSearchPattern,
    OUT PWSTR* ppwszFilesystemPath,
    OUT PWSTR* ppwszSearchPattern
    )
{
    NTSTATUS ntStatus = 0;
    wchar16_t wszStar[2] = {0, 0};
    wchar16_t wszBackslash[2] = {0, 0};
    wchar16_t wszQuestionMark[2] = {0, 0};
    wchar16_t wszQuote[2] = {0, 0};
    wchar16_t wszGT[2] = {0, 0};
    wchar16_t wszLT[2] = {0, 0};
    wchar16_t wszDot[2] = {0, 0};
    size_t    sLen = 0;
    PWSTR     pwszCursor = NULL;
    PWSTR     pwszLastSlash = NULL;
    PWSTR     pwszFilesystemPath = NULL;
    PWSTR     pwszSearchPattern3 = NULL;
    PWSTR     pwszSearchPattern2 = NULL;

    if (!pwszPath || !*pwszPath)
    {
        ntStatus = STATUS_INVALID_PARAMETER_1;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    wcstowc16s(&wszStar[0], L"*", 1);
    wcstowc16s(&wszBackslash[0], L"\\", 1);
    wcstowc16s(&wszQuestionMark[0], L"?", 1);
    wcstowc16s(&wszQuote[0], L"\"", 1);
    wcstowc16s(&wszGT[0], L">", 1);
    wcstowc16s(&wszLT[0], L"<", 1);
    wcstowc16s(&wszDot[0], L".", 1);

    sLen = wc16slen(pwszPath);

    while (pwszSearchPattern && *pwszSearchPattern && (*pwszSearchPattern == wszBackslash[0]))
    {
          pwszSearchPattern++;
    }

    if (pwszSearchPattern && *pwszSearchPattern)
    {

        ntStatus = SMBAllocateStringW(
                       pwszSearchPattern,
                       &pwszSearchPattern3);

        BAIL_ON_NT_STATUS(ntStatus);
    }

    pwszCursor = pwszSearchPattern3;
    while (pwszCursor && *pwszCursor)
    {
        if (*pwszCursor == wszGT[0])
        {
            *pwszCursor = wszQuestionMark[0];
        }
        else if (*pwszCursor == wszQuote[0])
        {
            PWSTR pwszNext = pwszCursor;

            pwszNext++;

            if (!pwszNext ||
                ((*pwszNext == wszQuestionMark[0] ||
                  *pwszNext == wszStar[0] ||
                  *pwszNext == wszGT[0] ||
                  *pwszNext == wszLT[0] ||
                  *pwszNext == wszQuote[0])))
            {
                *pwszCursor = wszDot[0];
            }
        }
        else if (*pwszCursor == wszLT[0])
        {
            PWSTR pwszNext = pwszCursor;

            pwszNext++;

            if (pwszNext ||
                (((*pwszNext == wszDot[0]) ||
                 (*pwszNext == wszQuote[0]) ||
                 (*pwszNext == wszLT[0]))))
            {
                *pwszCursor = wszStar[0];
            }
        }

        pwszCursor++;
    }

    pwszCursor = pwszSearchPattern3;

    while (pwszCursor && *pwszCursor)
    {
        if (*pwszCursor == wszBackslash[0])
        {
            pwszLastSlash = pwszCursor;
        }
        else if ((*pwszCursor == wszStar[0]) ||
                 (*pwszCursor == wszQuestionMark[0]))
        {
            break;
        }

        pwszCursor++;
    }

    if (pwszLastSlash)
    {
        PBYTE pDataCursor = NULL;
        size_t sSuffixLen = 0;

        sSuffixLen = ((PBYTE)pwszLastSlash - (PBYTE)pwszSearchPattern3);

        ntStatus = SrvAllocateMemory(
                        sLen * sizeof(wchar16_t) + sizeof(wszBackslash[0]) + sSuffixLen + sizeof(wchar16_t),
                        (PVOID*)&pwszFilesystemPath);
        BAIL_ON_NT_STATUS(ntStatus);

        pDataCursor = (PBYTE)pwszFilesystemPath;
        memcpy(pDataCursor, (PBYTE)pwszPath, sLen * sizeof(wchar16_t));
        pDataCursor += sLen * sizeof(wchar16_t);

        *((wchar16_t*)pDataCursor) = wszBackslash[0];
        pDataCursor += sizeof(wszBackslash[0]);

        memcpy(pDataCursor, pwszSearchPattern, sSuffixLen);
    }
    else
    {
        ntStatus = SMBAllocateStringW(
                        pwszPath,
                        &pwszFilesystemPath);
        BAIL_ON_NT_STATUS(ntStatus);
    }

    pwszCursor = (pwszLastSlash ? ++pwszLastSlash : pwszSearchPattern3);
    if (pwszCursor && *pwszCursor)
    {
        ntStatus = SMBAllocateStringW(
                        pwszCursor,
                        &pwszSearchPattern2);
        BAIL_ON_NT_STATUS(ntStatus);
    }
    else
    {
        ntStatus = SMBAllocateStringW(
                        wszStar,
                        &pwszSearchPattern2);
        BAIL_ON_NT_STATUS(ntStatus);
    }

    *ppwszFilesystemPath = pwszFilesystemPath;
    pwszFilesystemPath = NULL;

    *ppwszSearchPattern = pwszSearchPattern2;
    pwszSearchPattern2 = NULL;

cleanup:
    RTL_FREE(&pwszFilesystemPath);
    RTL_FREE(&pwszSearchPattern2);
    RTL_FREE(&pwszSearchPattern3);

    return ntStatus;

error:
    *ppwszFilesystemPath = NULL;
    *ppwszSearchPattern = NULL;

    goto cleanup;
}
