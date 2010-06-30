/*
 * Copyright Likewise Software    2004-2009
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

/*
 * Copyright (C) Likewise Software. All rights reserved.
 *
 * Module Name:
 *
 *        logfilter.c
 *
 * Abstract:
 *
 *        Likewise Input Output (LWIO) - SRV
 *
 *        Utilities
 *
 *        Log Filter
 *
 * Authors: Sriram Nambakam (snambakam@likewise.com)
 *
 */

/*
 * Log filter grammar
 *
 * <srv-log-filter> :=
 *              <srv-log-filter-client-ip>    ","
 *              <srv-log-filter-protocol-ver> ","
 *              <srv-log-filter-op-code>      ","
 *              <srv-log-filter-log-level>
 *
 * <srv-log-filter-log-level> :=
 *              "error"  | "warning" | "info" | "verbose" | "debug"
 *
 * <srv-log-filter-client-ip> :=
 *              "*" | <srv-log-filter-multi-client-ip>
 *
 * <srv-log-filter-multi-client-ip> :=
 *              "{" <srv-log-filter-ip> ( "," <srv-log-filter-ip>)* "}"
 *
 * <srv-log-filter-ip> :=
 *              <srv-log-filter-ipv4> | <srv-log-filter-ipv6>
 *
 * <srv-log-filter-op-code> :=
 *              "*" | <srv-log-filter-multi-op-code>
 *
 * <srv-log-filter-multi-op-code> :=
 *              "{" <srv-log-filter-op-code> ("," <srv-log-filter-op-code)* "}"
 *
 * <srv-log-filter-single-op> :=
 *              <srv-log-filter-protocol> ":" <srv-log-filter-op-code>
 *
 * <srv-log-filter-protocol-ver> :=
 *              "smb1" | "smb2"
 *
 * <srv-log-filter-op-code> :=
 *              <srv-log-filter-hex-op-code> | <srv-log-filter-decimal-op-code>
 *
 * <srv-log-filter-hex-op-code> :=
 *              ("0x"|"0X") [0-9A-Fa-f]+
 *
 * <srv-log-filter-decimal-op-code> :=
 *              [0-9]+
 */

#include "includes.h"

static
NTSTATUS
SrvLogSpecProcessFilter(
    PWSTR         pwszFilter,
    PSRV_LOG_SPEC pLogSpec
    );

static
NTSTATUS
SrvLogSpecParseFilter(
    PSTR             pszFilter,    /* IN     */
    PSRV_LOG_FILTER* ppLogFilter   /*    OUT */
    );

static
NTSTATUS
SrvLogSpecParseClients(
    PSRV_LOG_FILTER_LEX_STATE pLexState,   /* IN OUT */
    PSRV_LOG_FILTER*          ppLogFilter  /*    OUT */
    );

static
NTSTATUS
SrvLogSpecParseProtocol(
    PSRV_LOG_FILTER_LEX_STATE pLexState,      /* IN OUT */
    PULONG                    pulProtocolVer  /* IN OUT */
    );

static
NTSTATUS
SrvLogSpecParseOpcodes(
    PSRV_LOG_FILTER_LEX_STATE pLexState,     /* IN OUT */
    ULONG                     ulProtocolVer, /* IN     */
    PSRV_LOG_FILTER           pLogFilter     /* IN OUT */
    );

static
NTSTATUS
SrvLogSpecParseLoglevel(
    PSRV_LOG_FILTER_LEX_STATE pLexState,  /* IN OUT */
    PSRV_LOG_FILTER           pLogFilter  /*  IN OUT */
    );

static
NTSTATUS
SrvLogSpecGetNextToken(
    PSRV_LOG_FILTER_LEX_STATE pLexState,
    PSRV_LOG_FILTER_TOKEN     pToken
    );

static
NTSTATUS
SrvLogSpecMergeFilter(
    PSRV_LOG_SPEC    pLogSpec,     /* IN OUT */
    PSRV_LOG_FILTER* ppLogFilter   /* IN OUT */
    );

static
PSRV_LOG_FILTER
SrvLogSpecFindFilter(
    PSRV_LOG_FILTER  pFilterList,
    struct sockaddr* pClientAddress,
    SOCKLEN_T        ulClientAddressLength
    );

static
NTSTATUS
SrvLogSpecMergeOpFilters(
    PSRV_LOG_FILTER_OP  pSrcFilterOpList,
    PSRV_LOG_FILTER_OP* ppDestFilterOpList
    );

static
PSRV_LOG_FILTER_OP
SrvLogFilterFindOp(
    PSRV_LOG_FILTER_OP pFilterOpList,
    ULONG              ulOpcode
    );

static
NTSTATUS
SrvLogFilterOpListDuplicate(
    PSRV_LOG_FILTER_OP  pFilterOpList,
    PSRV_LOG_FILTER_OP* ppDuplicateList
    );

static
NTSTATUS
SrvLogFilterOpDuplicate(
    PSRV_LOG_FILTER_OP  pFilterOp,
    PSRV_LOG_FILTER_OP* ppDuplicate
    );

static
PSRV_LOG_FILTER_OP
SrvLogFilterOpListReverse(
    PSRV_LOG_FILTER_OP pLogFilterOpList
    );

static
VOID
SrvLogSpecFree(
    PSRV_LOG_SPEC pLogSpec
    );

static
VOID
SrvLogFilterFree(
    PSRV_LOG_FILTER pFilter
    );

static
PSRV_LOG_FILTER_OP
SrvLogFilterOpAcquire(
    PSRV_LOG_FILTER_OP pFilterOp
    );

static
VOID
SrvLogFilterOpListRelease(
    PSRV_LOG_FILTER_OP pFilterOpList
    );

static
VOID
SrvLogFilterOpRelease(
    PSRV_LOG_FILTER_OP pFilterOp
    );

static
VOID
SrvLogFilterOpFree(
    PSRV_LOG_FILTER_OP pFilterOp
    );

NTSTATUS
SrvLogSpecCreate(
    PSRV_LOG_SPEC* ppLogSpec
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    HANDLE   hRegConnection    = NULL;
    HKEY     hRootKey          = NULL;
    HKEY     hKey              = NULL;
    wchar16_t wszHKTM[]        = HKEY_THIS_MACHINE_W;
    wchar16_t wszLoggingKey[]  = REG_KEY_PATH_SRV_LOGGING_W;
    wchar16_t wszEnableLogging[] = REG_VALUE_SRV_LOGGING_ENABLED_W;
    DWORD     dwEnableLogging    = FALSE;
    DWORD     dwDataType         = REG_DWORD;
    DWORD     dwValueLen         = sizeof(dwEnableLogging);
    ULONG     ulValueLen         = MAX_VALUE_LENGTH;
    BYTE      pData[MAX_VALUE_LENGTH] = {0};
    PWSTR*    ppwszValues             = NULL;
    PSRV_LOG_SPEC pLogSpec = NULL;

    ntStatus = NtRegOpenServer(&hRegConnection);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = NtRegOpenKeyExW(
                    hRegConnection,
                    NULL,
                    &wszHKTM[0],
                    0,
                    KEY_READ,
                    &hRootKey);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = NtRegOpenKeyExW(
                    hRegConnection,
                    hRootKey,
                    &wszLoggingKey[0],
                    0,
                    KEY_READ,
                    &hKey);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = NtRegGetValueW(
                    hRegConnection,
                    hKey,
                    NULL,
                    &wszEnableLogging[0],
                    RRF_RT_REG_DWORD,
                    &dwDataType,
                    &dwEnableLogging,
                    &dwValueLen);
    BAIL_ON_NT_STATUS(ntStatus);

    if (dwEnableLogging)
    {
        wchar16_t wszFilters[] = REG_VALUE_SRV_FILTERS_W;
        INT       iValue       = 0;

        ntStatus = SrvAllocateMemory(sizeof(SRV_LOG_SPEC), (PVOID*)&pLogSpec);
        BAIL_ON_NT_STATUS(ntStatus);

        pLogSpec->refCount = 1;

        ntStatus = SrvAllocateMemory(
                        sizeof(SRV_LOG_FILTER),
                        (PVOID*)&pLogSpec->pDefaultSpec);
        BAIL_ON_NT_STATUS(ntStatus);

        pLogSpec->pDefaultSpec->defaultLogLevel = LWIO_LOG_LEVEL_ERROR;

        dwDataType = REG_MULTI_SZ;

        ntStatus = NtRegGetValueW(
                        hRegConnection,
                        hKey,
                        NULL,
                        &wszFilters[0],
                        RRF_RT_REG_MULTI_SZ,
                        &dwDataType,
                        pData,
                        &ulValueLen);
        BAIL_ON_NT_STATUS(ntStatus);

        ntStatus = NtRegByteArrayToMultiStrs(pData, ulValueLen, &ppwszValues);
        BAIL_ON_NT_STATUS(ntStatus);

        for (; ppwszValues[iValue]; iValue++)
        {
            PWSTR pwszValue = &ppwszValues[iValue][0];

            ntStatus = SrvLogSpecProcessFilter(pwszValue, pLogSpec);
            BAIL_ON_NT_STATUS(ntStatus);
        }
    }

    *ppLogSpec = pLogSpec;

cleanup:

    if (ppwszValues)
    {
        RegFreeMultiStrsW(ppwszValues);
    }

    if (hRegConnection)
    {
        if (hKey)
        {
            NtRegCloseKey(hRegConnection, hKey);
        }

        if (hRootKey)
        {
            NtRegCloseKey(hRegConnection, hRootKey);
        }

        NtRegCloseServer(hRegConnection);
    }

    return ntStatus;

error:

    *ppLogSpec = NULL;

    if (pLogSpec)
    {
        SrvLogSpecRelease(pLogSpec);
    }

    LWIO_LOG_ERROR("Failed to create enhanced logging specification."
                   " Error code: 0x%08x",
                   ntStatus);

    ntStatus = STATUS_SUCCESS;

    goto cleanup;
}

static
NTSTATUS
SrvLogSpecProcessFilter(
    PWSTR         pwszFilter,  /* IN     */
    PSRV_LOG_SPEC pLogSpec     /* IN OUT */
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PSTR     pszFilter = NULL;
    PSRV_LOG_FILTER pLogFilter = NULL;

    ntStatus = SrvWc16sToMbs(pwszFilter, &pszFilter);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SrvLogSpecParseFilter(pszFilter, &pLogFilter);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SrvLogSpecMergeFilter(pLogSpec, &pLogFilter);
    BAIL_ON_NT_STATUS(ntStatus);

cleanup:

    if (pLogFilter)
    {
        SrvLogFilterFree(pLogFilter);
    }

    SRV_SAFE_FREE_MEMORY(pszFilter);

    return ntStatus;

error:

    goto cleanup;
}

static
NTSTATUS
SrvLogSpecParseFilter(
    PSTR             pszFilter,    /* IN     */
    PSRV_LOG_FILTER* ppLogFilter   /*    OUT */
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PSRV_LOG_FILTER pLogFilter = NULL;
    ULONG           ulProtocolVer = 0;
    SRV_LOG_FILTER_LEX_STATE lexState =
    {
        .pszData   = pszFilter,
        .pszCursor = pszFilter
    };

    ntStatus = SrvLogSpecParseClients(&lexState, &pLogFilter);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SrvLogSpecParseProtocol(&lexState, &ulProtocolVer);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SrvLogSpecParseOpcodes(&lexState, ulProtocolVer, pLogFilter);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SrvLogSpecParseLoglevel(&lexState, pLogFilter);
    BAIL_ON_NT_STATUS(ntStatus);

    *ppLogFilter = pLogFilter;

cleanup:

    return ntStatus;

error:

    *ppLogFilter = NULL;

    if (pLogFilter)
    {
        SrvLogFilterFree(pLogFilter);
    }

    goto cleanup;
}

static
NTSTATUS
SrvLogSpecParseClients(
    PSRV_LOG_FILTER_LEX_STATE pLexState,       /* IN OUT */
    PSRV_LOG_FILTER*          ppLogFilterList  /*    OUT */
    )
{
    NTSTATUS        ntStatus   = STATUS_SUCCESS;
    PSRV_LOG_FILTER pLogFilterList = NULL;
    PSRV_LOG_FILTER pLogFilter     = NULL;
    SRV_LOG_FILTER_TOKEN token = {0};

    ntStatus = SrvLogSpecGetNextToken(pLexState, &token);
    BAIL_ON_NT_STATUS(ntStatus);

    switch (token.tokenType)
    {
        case SRV_LOG_FILTER_TOKEN_TYPE_STAR:

            ntStatus = SrvAllocateMemory(
                            sizeof(SRV_LOG_FILTER),
                            (PVOID*)&pLogFilterList);
            BAIL_ON_NT_STATUS(ntStatus);

            pLogFilterList->defaultLogLevel = LWIO_LOG_LEVEL_ERROR;

            break;

        case SRV_LOG_FILTER_TOKEN_TYPE_OPEN_BRACE:

            do
            {
                CHAR chSave = 0;

                ntStatus = SrvLogSpecGetNextToken(pLexState, &token);
                BAIL_ON_NT_STATUS(ntStatus);

                if (token.tokenType != SRV_LOG_FILTER_TOKEN_TYPE_PLAIN_TEXT)
                {
                    ntStatus = STATUS_DATA_ERROR;
                    BAIL_ON_NT_STATUS(ntStatus);
                }

                ntStatus = SrvAllocateMemory(
                                sizeof(SRV_LOG_FILTER),
                                (PVOID*)&pLogFilter);
                BAIL_ON_NT_STATUS(ntStatus);

                pLogFilter->defaultLogLevel = LWIO_LOG_LEVEL_ERROR;

                chSave = *(token.pData + token.ulLength);
                *(token.pData + token.ulLength) = 0;

                ntStatus = SrvSocketStringToAddressA(
                                token.pData,
                                &pLogFilter->clientAddress,
                                &pLogFilter->ulClientAddressLength);
                *(token.pData + token.ulLength) = chSave;

                BAIL_ON_NT_STATUS(ntStatus);

                pLogFilter->pNext = pLogFilterList;
                pLogFilterList = pLogFilter;
                pLogFilter = NULL;

                ntStatus = SrvLogSpecGetNextToken(pLexState, &token);
                BAIL_ON_NT_STATUS(ntStatus);

            } while (token.tokenType == SRV_LOG_FILTER_TOKEN_TYPE_COMMA);

            if (token.tokenType != SRV_LOG_FILTER_TOKEN_TYPE_CLOSE_BRACE)
            {
                ntStatus = STATUS_DATA_ERROR;
                BAIL_ON_NT_STATUS(ntStatus);
            }

            break;

        default:

            ntStatus = STATUS_DATA_ERROR;
            BAIL_ON_NT_STATUS(ntStatus);

            break;
    }

    if (pLogFilterList) // reverse
    {
        PSRV_LOG_FILTER pPrev = NULL;
        PSRV_LOG_FILTER pCur  = pLogFilterList;
        PSRV_LOG_FILTER pNext = NULL;

        /* pPrev->pCur->pNext */
        while (pCur)
        {
            pNext = pCur->pNext;
            pCur->pNext = pPrev;
            pPrev = pCur;
            pCur = pNext;
        }

        pLogFilterList = pPrev;
    }

    *ppLogFilterList = pLogFilterList;

cleanup:

    return ntStatus;

error:

    *ppLogFilterList = NULL;

    if (pLogFilterList)
    {
        SrvLogFilterFree(pLogFilterList);
    }

    if (pLogFilter)
    {
        SrvLogFilterFree(pLogFilter);
    }

    goto cleanup;
}

static
NTSTATUS
SrvLogSpecParseProtocol(
    PSRV_LOG_FILTER_LEX_STATE pLexState,      /* IN OUT */
    PULONG                    pulProtocolVer  /* IN OUT */
    )
{
    NTSTATUS        ntStatus   = STATUS_SUCCESS;
    SRV_LOG_FILTER_TOKEN token = {0};
    struct
    {
        ULONG protocolVer;
        PCSTR pszName;
        ULONG ulLength;
    }
    protocols[] =
    {
        { .protocolVer = 1, .pszName = "smb1", .ulLength = sizeof("smb1")-1 },
        { .protocolVer = 2, .pszName = "smb2", .ulLength = sizeof("smb2")-1 }
    };
    ULONG   ulNumProtocols = sizeof(protocols)/sizeof(protocols[0]);
    ULONG   iProtocol = 0;
    BOOLEAN bFound = FALSE;

    ntStatus = SrvLogSpecGetNextToken(pLexState, &token);
    BAIL_ON_NT_STATUS(ntStatus);

    if (token.tokenType != SRV_LOG_FILTER_TOKEN_TYPE_COMMA)
    {
        ntStatus = STATUS_DATA_ERROR;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    ntStatus = SrvLogSpecGetNextToken(pLexState, &token);
    BAIL_ON_NT_STATUS(ntStatus);

    if (token.tokenType != SRV_LOG_FILTER_TOKEN_TYPE_PLAIN_TEXT)
    {
        ntStatus = STATUS_DATA_ERROR;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    for (iProtocol = 0; iProtocol < ulNumProtocols; iProtocol++)
    {
        if (token.ulLength == protocols[iProtocol].ulLength &&
            !strncasecmp(   token.pData,
                            protocols[iProtocol].pszName,
                            protocols[iProtocol].ulLength))
        {
            *pulProtocolVer = protocols[iProtocol].protocolVer;

            bFound = TRUE;

            break;
        }
    }

    if (!bFound)
    {
        ntStatus = STATUS_DATA_ERROR;
        BAIL_ON_NT_STATUS(ntStatus);
    }

cleanup:

    return ntStatus;

error:

    *pulProtocolVer = 0;

    goto cleanup;
}

static
NTSTATUS
SrvLogSpecParseOpcodes(
    PSRV_LOG_FILTER_LEX_STATE pLexState,     /* IN OUT */
    ULONG                     ulProtocolVer, /* IN     */
    PSRV_LOG_FILTER           pLogFilter     /* IN OUT */
    )
{
    NTSTATUS             ntStatus   = STATUS_SUCCESS;
    PSRV_LOG_FILTER_OP   pLogOpFilterList = NULL;
    PSRV_LOG_FILTER_OP   pLogOpFilter     = NULL;
    SRV_LOG_FILTER_TOKEN token = {0};

    ntStatus = SrvLogSpecGetNextToken(pLexState, &token);
    BAIL_ON_NT_STATUS(ntStatus);

    if (token.tokenType != SRV_LOG_FILTER_TOKEN_TYPE_COMMA)
    {
        ntStatus = STATUS_DATA_ERROR;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    ntStatus = SrvLogSpecGetNextToken(pLexState, &token);
    BAIL_ON_NT_STATUS(ntStatus);

    switch (token.tokenType)
    {
        case SRV_LOG_FILTER_TOKEN_TYPE_STAR:

            break;

        case SRV_LOG_FILTER_TOKEN_TYPE_OPEN_BRACE:

            do
            {
                ULONG ulOpcode = 0;
                PSTR  pszEnd   = NULL;
                CHAR  chSave   = 0;

                ntStatus = SrvLogSpecGetNextToken(pLexState, &token);
                BAIL_ON_NT_STATUS(ntStatus);

                if (token.tokenType != SRV_LOG_FILTER_TOKEN_TYPE_PLAIN_TEXT)
                {
                    ntStatus = STATUS_DATA_ERROR;
                    BAIL_ON_NT_STATUS(ntStatus);
                }

                chSave = *(token.pData + token.ulLength);
                *(token.pData + token.ulLength) = 0;
                if ((token.ulLength > (sizeof("0x")-1)) &&
                    !strncasecmp(token.pData, "0x", sizeof("0x")-1))
                {
                    ulOpcode = strtoul(token.pData, &pszEnd, 16);
                }
                else
                {
                    ulOpcode = strtoul(token.pData, &pszEnd, 0);
                }

                if (*token.pData && pszEnd && !*pszEnd) // successful parse
                {
                    *(token.pData + token.ulLength) = chSave;
                }
                else // some characters could not be parsed
                {
                    *(token.pData + token.ulLength) = chSave;

                    ntStatus = STATUS_DATA_ERROR;
                    BAIL_ON_NT_STATUS(ntStatus);
                }

                // check for duplicates
                if (!SrvLogFilterFindOp(pLogOpFilterList, ulOpcode))
                {
                    ntStatus = SrvAllocateMemory(
                                    sizeof(SRV_LOG_FILTER_OP),
                                    (PVOID*)&pLogOpFilter);
                    BAIL_ON_NT_STATUS(ntStatus);

                    pLogOpFilter->refCount = 1;

                    pLogOpFilter->logLevel = LWIO_LOG_LEVEL_ERROR;

                    pLogOpFilter->ulOpcode = ulOpcode;

                    pLogOpFilter->pNext = pLogOpFilterList;
                    pLogOpFilterList = pLogOpFilter;
                    pLogOpFilter = NULL;
                }

                ntStatus = SrvLogSpecGetNextToken(pLexState, &token);
                BAIL_ON_NT_STATUS(ntStatus);

            } while (token.tokenType == SRV_LOG_FILTER_TOKEN_TYPE_COMMA);

            if (token.tokenType != SRV_LOG_FILTER_TOKEN_TYPE_CLOSE_BRACE)
            {
                ntStatus = STATUS_DATA_ERROR;
                BAIL_ON_NT_STATUS(ntStatus);
            }

            break;

        default:

            ntStatus = STATUS_DATA_ERROR;
            BAIL_ON_NT_STATUS(ntStatus);

            break;
    }

    if (pLogOpFilterList)
    {
        PSRV_LOG_FILTER pCursor = pLogFilter;

        pLogOpFilterList = SrvLogFilterOpListReverse(pLogOpFilterList);

        for (; pCursor; pCursor = pCursor->pNext)
        {
            PSRV_LOG_FILTER_OP pOpCursor = pLogOpFilterList;

            switch (ulProtocolVer)
            {
                case 1:

                    pCursor->pFilterList_smb1 = pLogOpFilterList;

                    break;

                case 2:

                    pCursor->pFilterList_smb2 = pLogOpFilterList;

                    break;

                default:

                    ntStatus = STATUS_DATA_ERROR;

                    break;
            }
            BAIL_ON_NT_STATUS(ntStatus);

            for (; pOpCursor; pOpCursor = pOpCursor->pNext)
            {
                SrvLogFilterOpAcquire(pOpCursor);
            }
        }
    }

cleanup:

    if (pLogOpFilterList)
    {
        SrvLogFilterOpListRelease(pLogOpFilterList);
    }

    if (pLogOpFilter)
    {
        SrvLogFilterOpRelease(pLogOpFilter);
    }

    return ntStatus;

error:

    goto cleanup;
}

static
NTSTATUS
SrvLogSpecParseLoglevel(
    PSRV_LOG_FILTER_LEX_STATE pLexState,  /* IN OUT */
    PSRV_LOG_FILTER           pLogFilter  /* IN OUT */
    )
{
    NTSTATUS        ntStatus   = STATUS_SUCCESS;
    SRV_LOG_FILTER_TOKEN token = {0};
    struct
    {
        LWIO_LOG_LEVEL logLevel;
        PCSTR          pszLogLevel;
        ULONG          ulLength;
    }
    logLevels[] =
    {
        {
                .logLevel    = LWIO_LOG_LEVEL_ERROR,
                .pszLogLevel = "error",
                .ulLength    = sizeof("error")-1
        },
        {
                .logLevel    = LWIO_LOG_LEVEL_WARNING,
                .pszLogLevel = "warning",
                .ulLength    = sizeof("warning")-1
        },
        {
                .logLevel    = LWIO_LOG_LEVEL_INFO,
                .pszLogLevel = "info",
                .ulLength    = sizeof("info")-1
        },
        {
                .logLevel    = LWIO_LOG_LEVEL_VERBOSE,
                .pszLogLevel = "verbose",
                .ulLength    = sizeof("verbose")-1
        },
        {
                .logLevel    = LWIO_LOG_LEVEL_DEBUG,
                .pszLogLevel = "debug",
                .ulLength    = sizeof("debug")-1
        }
    };
    ULONG   ulNumLevels = sizeof(logLevels)/sizeof(logLevels[0]);
    ULONG   iLevel = 0;
    BOOLEAN bFound = FALSE;

    ntStatus = SrvLogSpecGetNextToken(pLexState, &token);
    BAIL_ON_NT_STATUS(ntStatus);

    if (token.tokenType != SRV_LOG_FILTER_TOKEN_TYPE_COMMA)
    {
        ntStatus = STATUS_DATA_ERROR;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    ntStatus = SrvLogSpecGetNextToken(pLexState, &token);
    BAIL_ON_NT_STATUS(ntStatus);

    if (token.tokenType != SRV_LOG_FILTER_TOKEN_TYPE_PLAIN_TEXT)
    {
        ntStatus = STATUS_DATA_ERROR;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    for (iLevel = 0; iLevel < ulNumLevels; iLevel++)
    {
        if (token.ulLength == logLevels[iLevel].ulLength &&
            !strncasecmp(   (PCSTR)token.pData,
                            logLevels[iLevel].pszLogLevel,
                            logLevels[iLevel].ulLength))
        {
            PSRV_LOG_FILTER pCursor = pLogFilter;

            for (; pCursor; pCursor = pCursor->pNext)
            {
                PSRV_LOG_FILTER_OP pOpCursor = NULL;

                if (logLevels[iLevel].logLevel > pCursor->defaultLogLevel)
                {
                    pCursor->defaultLogLevel = logLevels[iLevel].logLevel;
                }

                for (pOpCursor = pCursor->pFilterList_smb1;
                     pOpCursor;
                     pOpCursor = pOpCursor->pNext)
                {
                    if (logLevels[iLevel].logLevel > pOpCursor->logLevel)
                    {
                        pOpCursor->logLevel = logLevels[iLevel].logLevel;
                    }
                }

                for (pOpCursor = pCursor->pFilterList_smb2;
                     pOpCursor;
                     pOpCursor = pOpCursor->pNext)
                {
                    if (logLevels[iLevel].logLevel > pOpCursor->logLevel)
                    {
                        pOpCursor->logLevel = logLevels[iLevel].logLevel;
                    }
                }
            }

            bFound = TRUE;

            break;
        }
    }

    if (!bFound)
    {
        ntStatus = STATUS_DATA_ERROR;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    ntStatus = SrvLogSpecGetNextToken(pLexState, &token);
    BAIL_ON_NT_STATUS(ntStatus);

    if (token.tokenType != SRV_LOG_FILTER_TOKEN_TYPE_EOF)
    {
        // extraneous data in buffer
        ntStatus = STATUS_DATA_ERROR;
        BAIL_ON_NT_STATUS(ntStatus);
    }

cleanup:

    return ntStatus;

error:

    goto cleanup;
}

static
NTSTATUS
SrvLogSpecGetNextToken(
    PSRV_LOG_FILTER_LEX_STATE pLexState,  /* IN OUT */
    PSRV_LOG_FILTER_TOKEN     pToken      /* IN OUT */
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;

    memset(pToken, 0, sizeof(*pToken));

    while (pLexState->pszCursor &&
           *pLexState->pszCursor &&
           isspace((int)(*pLexState->pszCursor)))
    {
        pLexState->pszCursor++;
    }

    if (!pLexState->pszCursor || !*pLexState->pszCursor)
    {
        pToken->tokenType = SRV_LOG_FILTER_TOKEN_TYPE_EOF;
    }
    else
    {
        switch (*pLexState->pszCursor)
        {
            case '*':

                pToken->tokenType = SRV_LOG_FILTER_TOKEN_TYPE_STAR;
                pToken->pData     = pLexState->pszCursor++;
                pToken->ulLength  = 1;

                break;

            case '{':

                pToken->tokenType = SRV_LOG_FILTER_TOKEN_TYPE_OPEN_BRACE;
                pToken->pData     = pLexState->pszCursor++;
                pToken->ulLength  = 1;

                break;

            case '}':

                pToken->tokenType = SRV_LOG_FILTER_TOKEN_TYPE_CLOSE_BRACE;
                pToken->pData     = pLexState->pszCursor++;
                pToken->ulLength  = 1;

                break;

            case ',':

                pToken->tokenType = SRV_LOG_FILTER_TOKEN_TYPE_COMMA;
                pToken->pData     = pLexState->pszCursor++;
                pToken->ulLength  = 1;

                break;

            default:

                {
                    BOOLEAN bDone = FALSE;

                    pToken->tokenType = SRV_LOG_FILTER_TOKEN_TYPE_PLAIN_TEXT;
                    pToken->pData     = pLexState->pszCursor;

                    while (!bDone &&
                            pLexState->pszCursor  &&
                           *pLexState->pszCursor &&
                           !isspace((int)(*pLexState->pszCursor)))
                    {
                        switch (*pLexState->pszCursor)
                        {
                            case '*':
                            case '{':
                            case '}':
                            case ',':

                                bDone = TRUE;

                                break;

                            default:

                                pLexState->pszCursor++;
                                pToken->ulLength++;

                                break;
                        }
                    }
                }

                break;
        }
    }

    return ntStatus;
}

static
NTSTATUS
SrvLogSpecMergeFilter(
    PSRV_LOG_SPEC    pLogSpec,     /* IN OUT */
    PSRV_LOG_FILTER* ppLogFilter   /* IN OUT */
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PSRV_LOG_FILTER pLogFilter = *ppLogFilter;
    PSRV_LOG_FILTER pCandidate = NULL;

    *ppLogFilter = NULL; // take ownership

    while (pLogFilter)
    {
        PSRV_LOG_FILTER pExisting = NULL;

        pCandidate = pLogFilter;
        pLogFilter = pLogFilter->pNext; // advance
        pCandidate->pNext = NULL;       // detach

        if (!pCandidate->ulClientAddressLength) // default filter
        {
            pExisting = pLogSpec->pDefaultSpec;
        }
        else
        {
            pExisting = SrvLogSpecFindFilter(
                            pLogSpec->pClientSpecList,
                            &pCandidate->clientAddress,
                            pCandidate->ulClientAddressLength);
        }

        if (!pExisting)
        {
            pCandidate->pNext = pLogSpec->pClientSpecList;
            pLogSpec->pClientSpecList = pCandidate;
            pCandidate = NULL;
        }
        else // merge
        {
            if (pCandidate->defaultLogLevel > pExisting->defaultLogLevel)
            {
                pExisting->defaultLogLevel = pCandidate->defaultLogLevel;
            }

            if (pCandidate->pFilterList_smb1)
            {
                ntStatus = SrvLogSpecMergeOpFilters(
                                    pCandidate->pFilterList_smb1,
                                    &pExisting->pFilterList_smb1);
                BAIL_ON_NT_STATUS(ntStatus);
            }

            if (pCandidate->pFilterList_smb2)
            {
                ntStatus = SrvLogSpecMergeOpFilters(
                                    pCandidate->pFilterList_smb2,
                                    &pExisting->pFilterList_smb2);
                BAIL_ON_NT_STATUS(ntStatus);
            }

            SrvLogFilterFree(pCandidate);
            pCandidate = NULL;
        }
    }

cleanup:

    if (pCandidate)
    {
        SrvLogFilterFree(pCandidate);
    }

    while (pLogFilter)
    {
        PSRV_LOG_FILTER pFilter1 = pLogFilter;

        pLogFilter = pLogFilter->pNext;

        SrvLogFilterFree(pFilter1);
    }

    return ntStatus;

error:

    goto cleanup;
}

static
PSRV_LOG_FILTER
SrvLogSpecFindFilter(
    PSRV_LOG_FILTER  pFilterList,
    struct sockaddr* pClientAddress,
    SOCKLEN_T        ulClientAddressLength
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PSRV_LOG_FILTER pCursor = pFilterList;

    for (; pCursor; pCursor = pCursor->pNext)
    {
        BOOLEAN bMatch = FALSE;

        ntStatus = SrvSocketCompareAddress(
                        &pCursor->clientAddress,
                        pCursor->ulClientAddressLength,
                        pClientAddress,
                        ulClientAddressLength,
                        &bMatch);
        BAIL_ON_NT_STATUS(ntStatus);

        if (bMatch)
        {
            break;
        }
    }

cleanup:

    return pCursor;

error:

    pCursor = NULL;

    goto cleanup;
}

static
NTSTATUS
SrvLogSpecMergeOpFilters(
    PSRV_LOG_FILTER_OP  pSrcFilterOpList,
    PSRV_LOG_FILTER_OP* ppDestFilterOpList
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;

    if (!*ppDestFilterOpList)
    {
        ntStatus = SrvLogFilterOpListDuplicate(
                        pSrcFilterOpList,
                        ppDestFilterOpList);
        BAIL_ON_NT_STATUS(ntStatus);
    }
    else
    {
        for (; pSrcFilterOpList; pSrcFilterOpList = pSrcFilterOpList->pNext)
        {
            PSRV_LOG_FILTER_OP pExisting  = NULL;

            pExisting = SrvLogFilterFindOp(
                            *ppDestFilterOpList,
                            pSrcFilterOpList->ulOpcode);

            if (!pExisting)
            {
                PSRV_LOG_FILTER_OP pDuplicate = NULL;

                // Make duplicates so that if the log level is over-ridden
                // by another rule, it is specific to a particular client
                //
                ntStatus = SrvLogFilterOpDuplicate(
                                pSrcFilterOpList,
                                &pDuplicate);
                BAIL_ON_NT_STATUS(ntStatus);

                pDuplicate->pNext   = *ppDestFilterOpList;
                *ppDestFilterOpList = pDuplicate;
            }
            else // merge
            {
                if (pSrcFilterOpList->logLevel > pExisting->logLevel)
                {
                    pExisting->logLevel = pSrcFilterOpList->logLevel;
                }
            }
        }
    }

error:

    return ntStatus;
}

static
PSRV_LOG_FILTER_OP
SrvLogFilterFindOp(
    PSRV_LOG_FILTER_OP pFilterOpList,
    ULONG              ulOpcode
    )
{
    PSRV_LOG_FILTER_OP pCursor = pFilterOpList;

    for (; pCursor; pCursor = pCursor->pNext)
    {
        if (pCursor->ulOpcode == ulOpcode)
        {
            break;
        }
    }

    return pCursor;
}

static
NTSTATUS
SrvLogFilterOpListDuplicate(
    PSRV_LOG_FILTER_OP  pFilterOpList,
    PSRV_LOG_FILTER_OP* ppDuplicateList
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PSRV_LOG_FILTER_OP pDuplicateList = NULL;

    for (; pFilterOpList; pFilterOpList = pFilterOpList->pNext)
    {
        PSRV_LOG_FILTER_OP pDuplicate = NULL;

        ntStatus = SrvLogFilterOpDuplicate(pFilterOpList, &pDuplicate);
        BAIL_ON_NT_STATUS(ntStatus);

        pDuplicate->pNext = pDuplicateList;
        pDuplicateList = pDuplicate;
    }

    *ppDuplicateList = SrvLogFilterOpListReverse(pDuplicateList);

cleanup:

    return ntStatus;

error:

    *ppDuplicateList = NULL;

    if (pDuplicateList)
    {
        SrvLogFilterOpListRelease(pDuplicateList);
    }

    goto cleanup;
}

static
NTSTATUS
SrvLogFilterOpDuplicate(
    PSRV_LOG_FILTER_OP  pFilterOp,
    PSRV_LOG_FILTER_OP* ppDuplicate
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PSRV_LOG_FILTER_OP pDuplicate = NULL;

    ntStatus = SrvAllocateMemory(
                    sizeof(SRV_LOG_FILTER_OP),
                    (PVOID*)&pDuplicate);
    BAIL_ON_NT_STATUS(ntStatus);

    pDuplicate->refCount = 1;

    pDuplicate->logLevel = pFilterOp->logLevel;
    pDuplicate->ulOpcode = pFilterOp->ulOpcode;

    *ppDuplicate = pDuplicate;

cleanup:

    return ntStatus;

error:

    *ppDuplicate = NULL;

    if (pDuplicate)
    {
        SrvLogFilterOpRelease(pDuplicate);
    }

    goto cleanup;
}

static
PSRV_LOG_FILTER_OP
SrvLogFilterOpListReverse(
    PSRV_LOG_FILTER_OP pLogFilterOpList
    )
{
    PSRV_LOG_FILTER_OP pPrev = NULL;
    PSRV_LOG_FILTER_OP pCur  = pLogFilterOpList;
    PSRV_LOG_FILTER_OP pNext = NULL;

    /* pPrev->pCur->pNext */
    while (pCur) /* reverse */
    {
        pNext = pCur->pNext;
        pCur->pNext = pPrev;
        pPrev = pCur;
        pCur = pNext;
    }

    return pPrev;
}

PSRV_LOG_SPEC
SrvLogSpecAcquire(
    PSRV_LOG_SPEC pLogSpec
    )
{
    InterlockedIncrement(&pLogSpec->refCount);

    return pLogSpec;
}

VOID
SrvLogSpecRelease(
    PSRV_LOG_SPEC pLogSpec
    )
{
    if (InterlockedDecrement(&pLogSpec->refCount) == 0)
    {
        SrvLogSpecFree(pLogSpec);
    }
}

NTSTATUS
SrvLogContextCreate(
    PSRV_LOG_CONTEXT* ppLogContext
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    BOOLEAN  bInLock  = FALSE;
    PSRV_LOG_CONTEXT pLogContext = NULL;

    ntStatus = SrvAllocateMemory(sizeof(SRV_LOG_CONTEXT), (PVOID*)&pLogContext);
    BAIL_ON_NT_STATUS(ntStatus);

    pthread_rwlock_init(&pLogContext->mutex, NULL);
    pLogContext->pMutex = &pLogContext->mutex;

    LWIO_LOCK_RWMUTEX_SHARED(bInLock, &gSrvUtilsGlobals.mutex);

    if (gSrvUtilsGlobals.pLogSpec)
    {
        pLogContext->pLogSpec = SrvLogSpecAcquire(gSrvUtilsGlobals.pLogSpec);

        pLogContext->pCurFilter = pLogContext->pLogSpec->pDefaultSpec;
    }

    *ppLogContext = pLogContext;

cleanup:

    LWIO_UNLOCK_RWMUTEX(bInLock, &gSrvUtilsGlobals.mutex);

    return ntStatus;

error:

    *ppLogContext = NULL;

    if (pLogContext)
    {
        SrvLogContextFree(pLogContext);
    }

    goto cleanup;
}

NTSTATUS
SrvLogContextUpdateFilter(
    PSRV_LOG_CONTEXT pLogContext,
    struct sockaddr* pClientAddress,
    SOCKLEN_T        ulClientAddressLength
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    BOOLEAN         bInLock = FALSE;

    LWIO_LOCK_RWMUTEX_EXCLUSIVE(bInLock, &pLogContext->mutex);

    if (pClientAddress && pLogContext->pLogSpec)
    {
        PSRV_LOG_FILTER pLogFilter = SrvLogSpecFindFilter(
                                        pLogContext->pLogSpec->pClientSpecList,
                                        pClientAddress,
                                        ulClientAddressLength);
        if (pLogFilter)
        {
            pLogContext->pCurFilter = pLogFilter;
        }
    }

    LWIO_UNLOCK_RWMUTEX(bInLock, &pLogContext->mutex);

    return ntStatus;
}

LWIO_LOG_LEVEL
SrvLogContextGetLevel(
    PSRV_LOG_CONTEXT pLogContext,
    ULONG            protocolVer,
    USHORT           usOpcode
    )
{
    LWIO_LOG_LEVEL logLevel = LWIO_LOG_LEVEL_ERROR;
    BOOLEAN        bInLock  = FALSE;

    LWIO_LOCK_RWMUTEX_SHARED(bInLock, &pLogContext->mutex);

    if (pLogContext->pCurFilter)
    {
        PSRV_LOG_FILTER_OP pFilterOp = NULL;

        switch (protocolVer)
        {
            case 1:

                if (pLogContext->pCurFilter->pFilterList_smb1)
                {
                    pFilterOp = SrvLogFilterFindOp(
                                    pLogContext->pCurFilter->pFilterList_smb1,
                                    usOpcode);
                }

                if (pFilterOp)
                {
                    logLevel = pFilterOp->logLevel;
                }
                else
                {
                    logLevel = pLogContext->pCurFilter->defaultLogLevel;
                }

                break;

            case 2:

                if (pLogContext->pCurFilter->pFilterList_smb2)
                {
                    pFilterOp = SrvLogFilterFindOp(
                                    pLogContext->pCurFilter->pFilterList_smb2,
                                    usOpcode);
                }

                if (pFilterOp)
                {
                    logLevel = pFilterOp->logLevel;
                }
                else
                {
                    logLevel = pLogContext->pCurFilter->defaultLogLevel;
                }

                break;

            default:

                logLevel = pLogContext->pCurFilter->defaultLogLevel;

                break;
        }
    }

    LWIO_UNLOCK_RWMUTEX(bInLock, &pLogContext->mutex);

    return logLevel;
}

VOID
SrvLogContextFree(
    PSRV_LOG_CONTEXT pLogContext
    )
{
    if (pLogContext->pLogSpec)
    {
        SrvLogSpecRelease(pLogContext->pLogSpec);
    }

    if (pLogContext->pMutex)
    {
        pthread_rwlock_destroy(&pLogContext->mutex);
    }

    SrvFreeMemory(pLogContext);
}

static
VOID
SrvLogSpecFree(
    PSRV_LOG_SPEC pLogSpec
    )
{
    while (pLogSpec->pClientSpecList)
    {
        PSRV_LOG_FILTER pFilter = pLogSpec->pClientSpecList;
        pLogSpec->pClientSpecList = pLogSpec->pClientSpecList->pNext;

        SrvLogFilterFree(pFilter);
    }

    if (pLogSpec->pDefaultSpec)
    {
        SrvLogFilterFree(pLogSpec->pDefaultSpec);
    }

    SrvFreeMemory(pLogSpec);
}

static
VOID
SrvLogFilterFree(
    PSRV_LOG_FILTER pFilter
    )
{
    if (pFilter->pFilterList_smb1)
    {
        SrvLogFilterOpListRelease(pFilter->pFilterList_smb1);
    }

    if (pFilter->pFilterList_smb2)
    {
        SrvLogFilterOpListRelease(pFilter->pFilterList_smb2);
    }

    SrvFreeMemory(pFilter);
}

static
PSRV_LOG_FILTER_OP
SrvLogFilterOpAcquire(
    PSRV_LOG_FILTER_OP pFilterOp
    )
{
    InterlockedIncrement(&pFilterOp->refCount);

    return pFilterOp;
}

static
VOID
SrvLogFilterOpListRelease(
    PSRV_LOG_FILTER_OP pFilterOpList
    )
{
    while (pFilterOpList)
    {
        PSRV_LOG_FILTER_OP pFilter = pFilterOpList;

        pFilterOpList = pFilterOpList->pNext;

        SrvLogFilterOpRelease(pFilter);
    }
}

static
VOID
SrvLogFilterOpRelease(
    PSRV_LOG_FILTER_OP pFilterOp
    )
{
    if (InterlockedDecrement(&pFilterOp->refCount) == 0)
    {
        SrvLogFilterOpFree(pFilterOp);
    }
}

static
VOID
SrvLogFilterOpFree(
    PSRV_LOG_FILTER_OP pFilterOp
    )
{
    SrvFreeMemory(pFilterOp);
}

