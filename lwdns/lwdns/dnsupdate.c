/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

/*
 * Copyright Likewise Software
 * All rights reserved.
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the license, or (at
 * your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser
 * General Public License for more details.  You should have received a copy
 * of the GNU Lesser General Public License along with this program.  If
 * not, see <http://www.gnu.org/licenses/>.
 *
 * LIKEWISE SOFTWARE MAKES THIS SOFTWARE AVAILABLE UNDER OTHER LICENSING
 * TERMS AS WELL.  IF YOU HAVE ENTERED INTO A SEPARATE LICENSE AGREEMENT
 * WITH LIKEWISE SOFTWARE, THEN YOU MAY ELECT TO USE THE SOFTWARE UNDER THE
 * TERMS OF THAT SOFTWARE LICENSE AGREEMENT INSTEAD OF THE TERMS OF THE GNU
 * LESSER GENERAL PUBLIC LICENSE, NOTWITHSTANDING THE ABOVE NOTICE.  IF YOU
 * HAVE QUESTIONS, OR WISH TO REQUEST A COPY OF THE ALTERNATE LICENSING
 * TERMS OFFERED BY LIKEWISE SOFTWARE, PLEASE CONTACT LIKEWISE SOFTWARE AT
 * license@likewisesoftware.com
 */

#include  "includes.h"

DWORD
DNSUpdateCreatePtrRUpdateRequest(
    PDNS_UPDATE_REQUEST* ppDNSUpdateRequest,
    PCSTR pszZoneName,
    PCSTR pszPtrName,
    PCSTR pszHostnameFQDN
    )
{
    DWORD dwError = 0;
    PDNS_UPDATE_REQUEST pDNSUpdateRequest = NULL;
    PDNS_ZONE_RECORD pDNSZoneRecord = NULL;
    PDNS_RR_RECORD pDNSPtrRecord = NULL;

    // Allocate pDNSUpdateRequest and fill in wIdentification and wParameter
    dwError = DNSUpdateCreateUpdateRequest(
                    &pDNSUpdateRequest);
    BAIL_ON_LWDNS_ERROR(dwError);

    dwError = DNSCreateZoneRecord(
                        pszZoneName,
                        &pDNSZoneRecord);
    BAIL_ON_LWDNS_ERROR(dwError);

    dwError = DNSUpdateAddZoneSection(
                        pDNSUpdateRequest,
                        pDNSZoneRecord);
    BAIL_ON_LWDNS_ERROR(dwError);

    pDNSZoneRecord = NULL;

    // Delete all PTR records associated with the fqdn.
    // This deletes hostnames that do not belong to the computer.
    dwError = DNSCreateDeleteRecord(
                    pszPtrName,
                    DNS_CLASS_ANY,
                    QTYPE_PTR,
                    &pDNSPtrRecord);
    BAIL_ON_LWDNS_ERROR(dwError);

    dwError = DNSUpdateAddUpdateSection(
                    pDNSUpdateRequest,
                    pDNSPtrRecord);
    BAIL_ON_LWDNS_ERROR(dwError);

    pDNSPtrRecord = NULL;

    dwError = DNSCreatePtrRecord(
                    pszPtrName,
                    DNS_CLASS_IN,
                    pszHostnameFQDN,
                    &pDNSPtrRecord);
    BAIL_ON_LWDNS_ERROR(dwError);

    dwError = DNSUpdateAddUpdateSection(
                    pDNSUpdateRequest,
                    pDNSPtrRecord);
    BAIL_ON_LWDNS_ERROR(dwError);

    pDNSPtrRecord = NULL;

    *ppDNSUpdateRequest = pDNSUpdateRequest;

cleanup:

    if (pDNSZoneRecord) {
        DNSFreeZoneRecord(pDNSZoneRecord);
    }

    if (pDNSPtrRecord)
    {
        DNSFreeRecord(pDNSPtrRecord);
    }

    return(dwError);

error:

    *ppDNSUpdateRequest = NULL;

    if (pDNSUpdateRequest) {
        DNSUpdateFreeRequest(pDNSUpdateRequest);
    }

    goto cleanup;
}

DWORD
DNSSendPtrUpdate(
    HANDLE hDNSServer,
    PCSTR  pszZoneName,
    PCSTR pszPtrName,
    PCSTR pszHostNameFQDN,
    PDNS_UPDATE_RESPONSE * ppDNSUpdateResponse
    )
{
    DWORD dwError = 0;
    PDNS_UPDATE_REQUEST  pDNSUpdateRequest = NULL;
    PDNS_UPDATE_RESPONSE pDNSUpdateResponse = NULL;
    PDNS_ZONE_RECORD pDNSZoneRecord = NULL;
    PDNS_RR_RECORD   pDNSARecord = NULL;

    LWDNS_LOG_INFO("Attempting DNS Update (in-secure)");

    dwError = DNSUpdateCreatePtrRUpdateRequest(
                    &pDNSUpdateRequest,
                    pszZoneName,
                    pszPtrName,
                    pszHostNameFQDN);
    BAIL_ON_LWDNS_ERROR(dwError);

    dwError = DNSUpdateSendUpdateRequest2(
                    hDNSServer,
                    pDNSUpdateRequest);
    BAIL_ON_LWDNS_ERROR(dwError);

    dwError = DNSUpdateReceiveUpdateResponse(
                    hDNSServer,
                    &pDNSUpdateResponse);
    BAIL_ON_LWDNS_ERROR(dwError);

    *ppDNSUpdateResponse = pDNSUpdateResponse;

    LWDNS_LOG_INFO("DNS Update (in-secure) succeeded");

cleanup:

    if (pDNSZoneRecord) {
        DNSFreeZoneRecord(pDNSZoneRecord);
    }

    if (pDNSARecord)
    {
        DNSFreeRecord(pDNSARecord);
    }

    if (pDNSUpdateRequest) {
        DNSUpdateFreeRequest(pDNSUpdateRequest);
    }

    return(dwError);

error:

    *ppDNSUpdateResponse = NULL;

    if (pDNSUpdateResponse) {
        DNSUpdateFreeResponse(pDNSUpdateResponse);
    }

    LWDNS_LOG_ERROR("DNS Update (in-secure) failed. [Error code:%d]", dwError);

    goto cleanup;
}

DWORD
DNSSendPtrSecureUpdate(
    HANDLE hDNSServer,
    PCtxtHandle pGSSContext,
    PCSTR pszKeyName,
    PCSTR pszZoneName,
    PCSTR pszPtrName,
    PCSTR pszHostNameFQDN,
    PDNS_UPDATE_RESPONSE * ppDNSUpdateResponse
    )
{
    DWORD dwError = 0;
    PDNS_UPDATE_REQUEST pDNSUpdateRequest = NULL;
    PDNS_UPDATE_RESPONSE pDNSUpdateResponse = NULL;

    LWDNS_LOG_INFO("Attempting DNS Update (secure)");

    dwError = DNSUpdateCreatePtrRUpdateRequest(
                    &pDNSUpdateRequest,
                    pszZoneName,
                    pszPtrName,
                    pszHostNameFQDN);
    BAIL_ON_LWDNS_ERROR(dwError);

    //
    // Now Sign the Record
    //
    dwError = DNSUpdateGenerateSignature(
                        pGSSContext,
                        pDNSUpdateRequest,
                        pszKeyName);
    BAIL_ON_LWDNS_ERROR(dwError);

    dwError = DNSUpdateSendUpdateRequest2(
                    hDNSServer,
                    pDNSUpdateRequest);
    BAIL_ON_LWDNS_ERROR(dwError);

    dwError = DNSUpdateReceiveUpdateResponse(
                    hDNSServer,
                    &pDNSUpdateResponse);
    BAIL_ON_LWDNS_ERROR(dwError);

    *ppDNSUpdateResponse = pDNSUpdateResponse;

    LWDNS_LOG_INFO("DNS Update (secure) succeeded");

cleanup:

    if (pDNSUpdateRequest) {
        DNSUpdateFreeRequest(pDNSUpdateRequest);
    }

    return(dwError);

error:

    if (pDNSUpdateResponse) {
        DNSUpdateFreeResponse(pDNSUpdateResponse);
    }

    *ppDNSUpdateResponse = NULL;

    LWDNS_LOG_ERROR("DNS Update (secure) failed. [Error code:%d]", dwError);

    goto cleanup;
}

DWORD
DNSUpdatePtrSecureOnServer(
    HANDLE hDNSServer,
    PCSTR  pszServerName,
    PCSTR  pszZoneName,
    PCSTR  pszPtrName,
    PCSTR  pszHostNameFQDN
    )
{
    DWORD dwError = 0;
    DWORD dwResponseCode = 0;
    PCSTR pszDomainName = strchr(pszServerName, '.');

    CtxtHandle GSSContext = {0};
    PCtxtHandle pGSSContext = &GSSContext;

    PDNS_UPDATE_RESPONSE pDNSUpdateResponse = NULL;
    PDNS_UPDATE_RESPONSE pDNSSecureUpdateResponse = NULL;
    PSTR pszKeyName = NULL;

    if (pszDomainName != NULL)
    {
        pszDomainName++;
    }
    else
    {
        dwError = LWDNS_ERROR_NO_SUCH_ZONE;
        BAIL_ON_LWDNS_ERROR(dwError);
    }

    dwError = DNSSendPtrUpdate(
                    hDNSServer,
                    pszZoneName,
                    pszPtrName,
                    pszHostNameFQDN,
                    &pDNSUpdateResponse);
    BAIL_ON_LWDNS_ERROR(dwError);

    dwError = DNSUpdateGetResponseCode(
                    pDNSUpdateResponse,
                    &dwResponseCode);
    BAIL_ON_LWDNS_ERROR(dwError);

    if (dwResponseCode == DNS_REFUSED) {

        dwError = DNSGenerateKeyName(&pszKeyName);
        BAIL_ON_LWDNS_ERROR(dwError);

        dwError = DNSNegotiateSecureContext(
                        hDNSServer,
                        pszDomainName,
                        pszServerName,
                        pszKeyName,
                        pGSSContext);
        BAIL_ON_LWDNS_ERROR(dwError);

        dwError = DNSSendPtrSecureUpdate(
                        hDNSServer,
                        pGSSContext,
                        pszKeyName,
                        pszZoneName,
                        pszPtrName,
                        pszHostNameFQDN,
                        &pDNSSecureUpdateResponse);
        BAIL_ON_LWDNS_ERROR(dwError);

        dwError = DNSUpdateGetResponseCode(
                    pDNSSecureUpdateResponse,
                    &dwResponseCode);
        BAIL_ON_LWDNS_ERROR(dwError);
    }

    dwError = DNSMapRCode(dwResponseCode);
    BAIL_ON_LWDNS_ERROR(dwError);

cleanup:

    if (*pGSSContext != GSS_C_NO_CONTEXT)
    {
        OM_uint32 dwMinorStatus = 0;

        gss_delete_sec_context(
            &dwMinorStatus,
            pGSSContext,
            GSS_C_NO_BUFFER);
    }

    if (pDNSUpdateResponse){
        DNSUpdateFreeResponse(pDNSUpdateResponse);
    }

    if (pDNSSecureUpdateResponse) {
        DNSUpdateFreeResponse(pDNSSecureUpdateResponse);
    }

    LWDNS_SAFE_FREE_STRING(pszKeyName);

    return dwError;

error:

    goto cleanup;
}

DWORD
DNSGetPtrNameForAddr(
    PSTR* ppszRecordName,
    PSOCKADDR_IN pAddr
    )
{
    DWORD dwError = 0;
    PSTR pszRecordName = NULL;

    if (pAddr->sin_family != AF_INET)
    {
        dwError = LWDNS_ERROR_INVALID_IP_ADDRESS;
        BAIL_ON_LWDNS_ERROR(dwError);
    }

    dwError = LwRtlCStringAllocatePrintf(
                    &pszRecordName,
                    "%d.%d.%d.%d.in-addr.arpa",
                    (pAddr->sin_addr.s_addr >> 24) & 255,
                    (pAddr->sin_addr.s_addr >> 16) & 255,
                    (pAddr->sin_addr.s_addr >>  8) & 255,
                    (pAddr->sin_addr.s_addr >>  0) & 255
                );
    if (dwError)
    {
        dwError = ENOMEM;
        BAIL_ON_LWDNS_ERROR(dwError);
    }

    *ppszRecordName = pszRecordName;

cleanup:
    return dwError;

error:
    *ppszRecordName = NULL;
    LWDNS_SAFE_FREE_STRING(pszRecordName);

    goto cleanup;
}

DWORD
DNSUpdatePtrSecure(
    PSOCKADDR_IN pAddr,
    PCSTR  pszHostnameFQDN
    )
{
    DWORD dwError = 0;
    PSTR   pszZone = NULL;
    PLW_NS_INFO pNameServerInfos = NULL;
    DWORD   dwNumNSInfos = 0;
    BOOLEAN bDNSUpdated = FALSE;
    PSTR pszRecordName = NULL;
    DWORD   iNS = 0;
    HANDLE hDNSServer = (HANDLE)NULL;
    PCSTR pszAddress = NULL;

    dwError = DNSGetPtrNameForAddr(
                    &pszRecordName,
                    pAddr);
    BAIL_ON_LWDNS_ERROR(dwError);

    dwError = DNSGetNameServers(
                    pszRecordName,
                    &pszZone,
                    &pNameServerInfos,
                    &dwNumNSInfos);
    BAIL_ON_LWDNS_ERROR(dwError);

    for (; !bDNSUpdated && (iNS < dwNumNSInfos); iNS++)
    {
        PSTR   pszNameServer = NULL;
        PLW_NS_INFO pNSInfo = NULL;

        pNSInfo = &pNameServerInfos[iNS];
        pszNameServer = pNSInfo->pszNSHostName;

        if (hDNSServer != (HANDLE)NULL)
        {
            DNSClose(hDNSServer);
        }

        pszAddress = inet_ntoa(pAddr->sin_addr);
        LWDNS_LOG_INFO("Attempting to update PTR record for %s to %s on name server [%s]", pszAddress, pszHostnameFQDN, pszNameServer);

        dwError = DNSOpen(
                        pszNameServer,
                        DNS_TCP,
                        &hDNSServer);
        if (dwError)
        {
            LWDNS_LOG_ERROR(
                    "Failed to open connection to Name Server [%s]. [Error code:%d]",
                    pszNameServer,
                    dwError);
            dwError = 0;

            continue;
        }

        dwError = DNSUpdatePtrSecureOnServer(
                        hDNSServer,
                        pszNameServer,
                        pszZone,
                        pszRecordName,
                        pszHostnameFQDN);
        if (dwError)
        {
            LWDNS_LOG_ERROR(
                    "Failed to update Name Server [%s]. [Error code:%d]",
                    pszNameServer,
                    dwError);
            dwError = 0;

            continue;
        }

        bDNSUpdated = TRUE;
    }

    if (!bDNSUpdated)
    {
        dwError = LWDNS_ERROR_UPDATE_FAILED;
        BAIL_ON_LWDNS_ERROR(dwError);
    }

cleanup:
    LWDNS_SAFE_FREE_STRING(pszZone);
    if (pNameServerInfos)
    {
        DNSFreeNameServerInfoArray(
                pNameServerInfos,
                dwNumNSInfos);
    }
    LWDNS_SAFE_FREE_STRING(pszRecordName);
    if (hDNSServer)
    {
        DNSClose(hDNSServer);
    }

    return dwError;

error:
    goto cleanup;
}

DWORD
DNSUpdateSecure(
    HANDLE hDNSServer,
    PCSTR  pszServerName,
    PCSTR  pszDomainName,
    PCSTR  pszHostNameFQDN,
    DWORD  dwNumAddrs,
    PSOCKADDR_IN pAddrArray
    )
{
    DWORD dwError = 0;
    DWORD dwResponseCode = 0;

    CtxtHandle GSSContext = {0};
    PCtxtHandle pGSSContext = &GSSContext;

    PDNS_UPDATE_RESPONSE pDNSUpdateResponse = NULL;
    PDNS_UPDATE_RESPONSE pDNSSecureUpdateResponse = NULL;
    PSTR pszKeyName = NULL;

    LWDNS_LOG_INFO("Attempting DNS Update (in-secure)");

    dwError = DNSSendUpdate(
                    hDNSServer,
                    pszDomainName,
                    pszHostNameFQDN,
                    dwNumAddrs,
                    pAddrArray,
                    &pDNSUpdateResponse);
    BAIL_ON_LWDNS_ERROR(dwError);

    dwError = DNSUpdateGetResponseCode(
                    pDNSUpdateResponse,
                    &dwResponseCode);
    BAIL_ON_LWDNS_ERROR(dwError);

    if (dwResponseCode == DNS_REFUSED)
    {
        LWDNS_LOG_INFO("DNS Update (in-secure) denied");

        dwError = DNSGenerateKeyName(&pszKeyName);
        BAIL_ON_LWDNS_ERROR(dwError);

        dwError = DNSNegotiateSecureContext(
                        hDNSServer,
                        pszDomainName,
                        pszServerName,
                        pszKeyName,
                        pGSSContext);
        BAIL_ON_LWDNS_ERROR(dwError);

        dwError = DNSSendSecureUpdate(
                        hDNSServer,
                        pGSSContext,
                        pszKeyName,
                        pszDomainName,
                        pszHostNameFQDN,
                        dwNumAddrs,
                        pAddrArray,
                        &pDNSSecureUpdateResponse);
        BAIL_ON_LWDNS_ERROR(dwError);

        dwError = DNSUpdateGetResponseCode(
                    pDNSSecureUpdateResponse,
                    &dwResponseCode);
        BAIL_ON_LWDNS_ERROR(dwError);

        dwError = DNSMapRCode(dwResponseCode);
        BAIL_ON_LWDNS_ERROR(dwError);
    }
    else
    {
        dwError = DNSMapRCode(dwResponseCode);
        BAIL_ON_LWDNS_ERROR(dwError);

        LWDNS_LOG_INFO("DNS Update (in-secure) succeeded");
    }

cleanup:

    if (*pGSSContext != GSS_C_NO_CONTEXT)
    {
        OM_uint32 dwMinorStatus = 0;

        gss_delete_sec_context(
            &dwMinorStatus,
            pGSSContext,
            GSS_C_NO_BUFFER);
    }

    if (pDNSUpdateResponse){
        DNSUpdateFreeResponse(pDNSUpdateResponse);
    }

    if (pDNSSecureUpdateResponse) {
        DNSUpdateFreeResponse(pDNSSecureUpdateResponse);
    }

    LWDNS_SAFE_FREE_STRING(pszKeyName);

    return dwError;

error:

    goto cleanup;
}

DWORD
DNSUpdateCreateARUpdateRequest(
    PDNS_UPDATE_REQUEST* ppDNSUpdateRequest,
    PCSTR pszZoneName,
    PCSTR pszHostnameFQDN,
    DWORD  dwNumAddrs,
    PSOCKADDR_IN pAddrArray
    )
{
    DWORD dwError = 0;
    PDNS_UPDATE_REQUEST pDNSUpdateRequest = NULL;
    PDNS_ZONE_RECORD pDNSZoneRecord = NULL;
    PDNS_RR_RECORD pDNSPRRecord = NULL;
    PDNS_RR_RECORD pDNSARecord = NULL;
    DWORD iAddr = 0;

    // Allocate pDNSUpdateRequest and fill in wIdentification and wParameter
    dwError = DNSUpdateCreateUpdateRequest(
                    &pDNSUpdateRequest);
    BAIL_ON_LWDNS_ERROR(dwError);

    dwError = DNSCreateZoneRecord(
                        pszZoneName,
                        &pDNSZoneRecord);
    BAIL_ON_LWDNS_ERROR(dwError);

    dwError = DNSUpdateAddZoneSection(
                        pDNSUpdateRequest,
                        pDNSZoneRecord);
    BAIL_ON_LWDNS_ERROR(dwError);

    pDNSZoneRecord = NULL;

    // Creates a prerequisite saying that the fqdn does not already exist as a
    // CNAME. The prequisite will pass if the record exists as another type
    // (such as an A record).
    // This prerequisite stops the tool from replacing a CNAME with an A
    // record.
    dwError = DNSCreateNameNotInUseRecord(
                    pszHostnameFQDN,
                    &pDNSPRRecord);
    BAIL_ON_LWDNS_ERROR(dwError);

    dwError = DNSUpdateAddPRSection(
                    pDNSUpdateRequest,
                    pDNSPRRecord);
    BAIL_ON_LWDNS_ERROR(dwError);

    pDNSPRRecord = NULL;

    // Delete all A records associated with the fqdn.
    // This deletes IP addresses that do not belong to the computer.
    dwError = DNSCreateDeleteRecord(
                    pszHostnameFQDN,
                    DNS_CLASS_ANY,
                    QTYPE_A,
                    &pDNSARecord);
    BAIL_ON_LWDNS_ERROR(dwError);

    dwError = DNSUpdateAddUpdateSection(
                    pDNSUpdateRequest,
                    pDNSARecord);
    BAIL_ON_LWDNS_ERROR(dwError);

    pDNSARecord = NULL;

    // Add an A record for every IP address that belongs to the computer. If
    // the delete operation above deleted IP addresses that actually belong to
    // the computer, this will recreate them.
    for (; iAddr < dwNumAddrs; iAddr++)
    {
        PSOCKADDR_IN pSockAddr = NULL;
        PCSTR pszAddress = NULL;
        
        pSockAddr = &pAddrArray[iAddr];

        pszAddress = inet_ntoa(pSockAddr->sin_addr);

        LWDNS_LOG_INFO("Adding IP Address [%s] to DNS Update request", pszAddress);

        dwError = DNSCreateARecord(
                        pszHostnameFQDN,
                        DNS_CLASS_IN,
                        QTYPE_A,
                        htonl(pSockAddr->sin_addr.s_addr),
                        &pDNSARecord);
        BAIL_ON_LWDNS_ERROR(dwError);

        dwError = DNSUpdateAddUpdateSection(
                        pDNSUpdateRequest,
                        pDNSARecord);
        BAIL_ON_LWDNS_ERROR(dwError);

        pDNSARecord = NULL;
    }

    *ppDNSUpdateRequest = pDNSUpdateRequest;

cleanup:

    if (pDNSZoneRecord) {
        DNSFreeZoneRecord(pDNSZoneRecord);
    }

    if (pDNSARecord)
    {
        DNSFreeRecord(pDNSARecord);
    }

    if (pDNSPRRecord)
    {
        DNSFreeRecord(pDNSPRRecord);
    }

    return(dwError);

error:

    *ppDNSUpdateRequest = NULL;

    if (pDNSUpdateRequest) {
        DNSUpdateFreeRequest(pDNSUpdateRequest);
    }

    goto cleanup;
}

DWORD
DNSSendUpdate(
    HANDLE hDNSServer,
    PCSTR  pszZoneName,
    PCSTR  pszHostnameFQDN,
    DWORD  dwNumAddrs,
    PSOCKADDR_IN pAddrArray,
    PDNS_UPDATE_RESPONSE * ppDNSUpdateResponse
    )
{
    DWORD dwError = 0;
    PDNS_UPDATE_REQUEST  pDNSUpdateRequest = NULL;
    PDNS_UPDATE_RESPONSE pDNSUpdateResponse = NULL;
    PDNS_ZONE_RECORD pDNSZoneRecord = NULL;
    PDNS_RR_RECORD   pDNSARecord = NULL;

    dwError = DNSUpdateCreateARUpdateRequest(
                    &pDNSUpdateRequest,
                    pszZoneName,
                    pszHostnameFQDN,
                    dwNumAddrs,
                    pAddrArray);
    BAIL_ON_LWDNS_ERROR(dwError);

    dwError = DNSUpdateSendUpdateRequest2(
                    hDNSServer,
                    pDNSUpdateRequest);
    BAIL_ON_LWDNS_ERROR(dwError);

    dwError = DNSUpdateReceiveUpdateResponse(
                    hDNSServer,
                    &pDNSUpdateResponse);
    BAIL_ON_LWDNS_ERROR(dwError);

    *ppDNSUpdateResponse = pDNSUpdateResponse;

cleanup:

    if (pDNSZoneRecord) {
        DNSFreeZoneRecord(pDNSZoneRecord);
    }

    if (pDNSARecord)
    {
        DNSFreeRecord(pDNSARecord);
    }

    if (pDNSUpdateRequest) {
        DNSUpdateFreeRequest(pDNSUpdateRequest);
    }

    return(dwError);

error:

    *ppDNSUpdateResponse = NULL;

    if (pDNSUpdateResponse) {
        DNSUpdateFreeResponse(pDNSUpdateResponse);
    }

    LWDNS_LOG_ERROR("DNS Update (in-secure) failed. [Error code:%d]", dwError);

    goto cleanup;
}

DWORD
DNSSendSecureUpdate(
    HANDLE hDNSServer,
    PCtxtHandle pGSSContext,
    PCSTR pszKeyName,
    PCSTR pszZoneName,
    PCSTR pszHostnameFQDN,
    DWORD  dwNumAddrs,
    PSOCKADDR_IN pAddrArray,
    PDNS_UPDATE_RESPONSE * ppDNSUpdateResponse
    )
{
    DWORD dwError = 0;
    PDNS_UPDATE_REQUEST pDNSUpdateRequest = NULL;
    PDNS_UPDATE_RESPONSE pDNSUpdateResponse = NULL;

    LWDNS_LOG_INFO("Attempting DNS Update (secure)");

    dwError = DNSUpdateCreateARUpdateRequest(
                    &pDNSUpdateRequest,
                    pszZoneName,
                    pszHostnameFQDN,
                    dwNumAddrs,
                    pAddrArray);
    BAIL_ON_LWDNS_ERROR(dwError);

    //
    // Now Sign the Record
    //
    dwError = DNSUpdateGenerateSignature(
                        pGSSContext,
                        pDNSUpdateRequest,
                        pszKeyName);
    BAIL_ON_LWDNS_ERROR(dwError);

    dwError = DNSUpdateSendUpdateRequest2(
                    hDNSServer,
                    pDNSUpdateRequest);
    BAIL_ON_LWDNS_ERROR(dwError);

    dwError = DNSUpdateReceiveUpdateResponse(
                    hDNSServer,
                    &pDNSUpdateResponse);
    BAIL_ON_LWDNS_ERROR(dwError);

    *ppDNSUpdateResponse = pDNSUpdateResponse;

    LWDNS_LOG_INFO("DNS Update (secure) succeeded");

cleanup:

    if (pDNSUpdateRequest) {
        DNSUpdateFreeRequest(pDNSUpdateRequest);
    }

    return(dwError);

error:

    if (pDNSUpdateResponse) {
        DNSUpdateFreeResponse(pDNSUpdateResponse);
    }

    *ppDNSUpdateResponse = NULL;

    LWDNS_LOG_ERROR("DNS Update (secure) failed. [Error code:%d]", dwError);

    goto cleanup;
}

DWORD
DNSUpdateGenerateSignature(
    PCtxtHandle pGSSContext,
    PDNS_UPDATE_REQUEST pDNSUpdateRequest,
    PCSTR pszKeyName
    )
{
    DWORD dwError = 0;
    DWORD dwMinorStatus = 0;
    HANDLE hSendBuffer = (HANDLE)NULL;
    PBYTE pMessageBuffer = NULL;
    DWORD dwMessageSize = 0;
    DWORD dwTimeSigned = 0;
    WORD wFudge = 0;
    gss_buffer_desc MsgDesc = {0};
    gss_buffer_desc MicDesc = {0};
    PDNS_RR_RECORD pDNSTSIGRecord = NULL;

    dwError = DNSBuildMessageBuffer(
                        pDNSUpdateRequest,
                        pszKeyName,
                        &dwTimeSigned,
                        &wFudge,
                        &pMessageBuffer,
                        &dwMessageSize);
    BAIL_ON_LWDNS_ERROR(dwError);

    MsgDesc.value = pMessageBuffer;
    MsgDesc.length = dwMessageSize;

    MicDesc.value = NULL;
    MicDesc.length = 0;

    dwError = gss_get_mic(
                    (OM_uint32 *)&dwMinorStatus,
                    *pGSSContext,
                    0,
                    &MsgDesc,
                    &MicDesc);
    lwdns_display_status("gss_init_context", dwError, dwMinorStatus);
    BAIL_ON_LWDNS_ERROR(dwError);

    dwError = DNSCreateTSIGRecord(
                    pszKeyName,
                    dwTimeSigned,
                    wFudge,
                    pDNSUpdateRequest->wIdentification,
                    MicDesc.value,
                    MicDesc.length,
                    &pDNSTSIGRecord);
    BAIL_ON_LWDNS_ERROR(dwError);

    dwError = DNSUpdateAddAdditionalSection(
                    pDNSUpdateRequest,
                    pDNSTSIGRecord);
    BAIL_ON_LWDNS_ERROR(dwError);

    pDNSTSIGRecord = NULL;

cleanup:

    gss_release_buffer(&dwMinorStatus, &MicDesc);

    if (pDNSTSIGRecord)
    {
        DNSFreeRecord(pDNSTSIGRecord);
    }

    if (hSendBuffer) {
        DNSFreeSendBufferContext(hSendBuffer);
    }

    if (pMessageBuffer) {
        DNSFreeMemory(pMessageBuffer);
    }

    return(dwError);

error:

    goto cleanup;
}

DWORD
DNSBuildMessageBuffer(
    PDNS_UPDATE_REQUEST pDNSUpdateRequest,
    PCSTR   pszKeyName,
    DWORD * pdwTimeSigned,
    WORD *  pwFudge,
    PBYTE * ppMessageBuffer,
    PDWORD  pdwMessageSize
    )
{
    DWORD dwError = 0;
    PBYTE pSrcBuffer = NULL;
    DWORD dwReqMsgSize = 0;
    DWORD dwAlgorithmLen = 0;
    DWORD dwNameLen = 0;
    PBYTE pMessageBuffer = NULL;
    DWORD dwMessageSize = 0;
    PBYTE pOffset = NULL;
    WORD wnError, wError = 0;
    WORD wnFudge = 0;
    WORD wFudge = DNS_ONE_HOUR_IN_SECS;
    WORD wnOtherLen = 0, wOtherLen = 0;
    DWORD dwBytesCopied = 0;
    WORD wnClass = 0, wClass = DNS_CLASS_ANY;
    DWORD dwnTTL = 0, dwTTL = 0;
    DWORD dwnTimeSigned, dwTimeSigned = 0;
    HANDLE hSendBuffer = (HANDLE)NULL;
    PDNS_DOMAIN_NAME pDomainName = NULL;
    PDNS_DOMAIN_NAME pAlgorithmName = NULL;
    //PBYTE pOtherLen = NULL;
    WORD wTimePrefix = 0;
    WORD wnTimePrefix = 0;

    dwError = DNSDomainNameFromString(pszKeyName, &pDomainName);
    BAIL_ON_LWDNS_ERROR(dwError);

    dwError = DNSGetDomainNameLength(pDomainName, &dwNameLen);
    BAIL_ON_LWDNS_ERROR(dwError);

    dwError = DNSDomainNameFromString(DNS_GSS_ALGORITHM, &pAlgorithmName);
    BAIL_ON_LWDNS_ERROR(dwError);

    dwError = DNSGetDomainNameLength(pAlgorithmName, &dwAlgorithmLen);
    BAIL_ON_LWDNS_ERROR(dwError);

    dwError = DNSUpdateBuildRequestMessage(
                pDNSUpdateRequest,
                &hSendBuffer);
    BAIL_ON_LWDNS_ERROR(dwError);

    dwReqMsgSize = DNSGetSendBufferContextSize(hSendBuffer);
    dwMessageSize += dwReqMsgSize;
    dwMessageSize += dwNameLen;
    dwMessageSize += sizeof(WORD); //CLASS
    dwMessageSize += sizeof(DWORD); //TTL
    dwMessageSize += dwAlgorithmLen;
    dwMessageSize += (sizeof(WORD) + sizeof(DWORD)); //Time Signed
    dwMessageSize += sizeof(WORD); //Fudge
    dwMessageSize += sizeof(WORD); //wError
    dwMessageSize += sizeof(WORD); //Other Len
    dwMessageSize += wOtherLen;

    dwError = DNSAllocateMemory(
                dwMessageSize,
                (PVOID *)&pMessageBuffer);
    BAIL_ON_LWDNS_ERROR(dwError);

    pOffset = pMessageBuffer;
    pSrcBuffer = DNSGetSendBufferContextBuffer(hSendBuffer);
    memcpy(pOffset, pSrcBuffer, dwReqMsgSize);
    pOffset += dwReqMsgSize;

    dwError = DNSCopyDomainName(pOffset, pDomainName, &dwBytesCopied);
    BAIL_ON_LWDNS_ERROR(dwError);
    pOffset +=  dwBytesCopied;

    wnClass = htons(wClass);
    memcpy(pOffset, &wnClass, sizeof(WORD));
    pOffset += sizeof(WORD);

    dwnTTL = htonl(dwTTL);
    memcpy(pOffset, &dwnTTL, sizeof(DWORD));
    pOffset += sizeof(DWORD);

    dwError = DNSCopyDomainName(pOffset, pAlgorithmName, &dwBytesCopied);
    BAIL_ON_LWDNS_ERROR(dwError);
    pOffset +=  dwBytesCopied;

    wnTimePrefix = htons(wTimePrefix);
    memcpy(pOffset, &wnTimePrefix, sizeof(WORD));
    pOffset += sizeof(WORD);

    time((time_t*)&dwTimeSigned);
    dwnTimeSigned = htonl(dwTimeSigned);
    memcpy(pOffset, &dwnTimeSigned, sizeof(DWORD));
    pOffset += sizeof(DWORD);

    wnFudge = htons(wFudge);
    memcpy(pOffset, &wnFudge, sizeof(WORD));
    pOffset += sizeof(WORD);

    wnError = htons(wError);
    memcpy(pOffset, &wnError, sizeof(WORD));
    pOffset += sizeof(WORD);

    wnOtherLen = htons(wOtherLen);
    memcpy(pOffset, &wnOtherLen, sizeof(WORD));
    pOffset += sizeof(WORD);

    *ppMessageBuffer = pMessageBuffer;
    *pdwMessageSize = dwMessageSize;

    *pdwTimeSigned = dwTimeSigned;
    *pwFudge = wFudge;

cleanup:

    if (pAlgorithmName)
    {
        DNSFreeDomainName(pAlgorithmName);
    }

    if (pDomainName)
    {
        DNSFreeDomainName(pDomainName);
    }

    if (hSendBuffer != (HANDLE)NULL)
    {
        DNSFreeSendBufferContext(hSendBuffer);
    }

    return(dwError);

error:

    if (pMessageBuffer) {
        DNSFreeMemory(pMessageBuffer);
    }

    *ppMessageBuffer = NULL;
    *pdwMessageSize = 0;
    *pdwTimeSigned = dwTimeSigned;
    *pwFudge = wFudge;

    goto cleanup;
}

