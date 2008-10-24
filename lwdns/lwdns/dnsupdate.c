
#include  "includes.h"

DWORD
DNSUpdateSecure(
    HANDLE hDNSServer,
    PCSTR  pszServerName,
    PCSTR  pszDomainName,
    PCSTR  pszHostNameFQDN,
    PSOCKADDR_IN pAddrArray,
    DWORD  dwNumAddrs
    )
{
    DWORD dwError = 0;
    DWORD dwResponseCode = 0;

    CtxtHandle GSSContext = {0};
    PCtxtHandle pGSSContext = &GSSContext;

    PDNS_UPDATE_RESPONSE pDNSUpdateResponse = NULL;
    PDNS_UPDATE_RESPONSE pDNSSecureUpdateResponse = NULL;
    PSTR pszKeyName = NULL;

    dwError = DNSSendUpdate(
                    hDNSServer,
                    pszDomainName,
                    pszHostNameFQDN,
                    pAddrArray,
                    dwNumAddrs,
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

        dwError = DNSSendSecureUpdate(
                        hDNSServer,
                        pGSSContext,
                        pszKeyName,
                        pszDomainName,
                        pszHostNameFQDN,
                        pAddrArray,
                        dwNumAddrs,
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
DNSSendUpdate(
    HANDLE hDNSServer,
    PCSTR  pszDomainName,
    PCSTR  pszHostnameFQDN,
    PSOCKADDR_IN pAddrArray,
    DWORD  dwNumAddrs,
    PDNS_UPDATE_RESPONSE * ppDNSUpdateResponse
    )
{
    DWORD dwError = 0;
    PDNS_UPDATE_REQUEST  pDNSUpdateRequest = NULL;
    PDNS_UPDATE_RESPONSE pDNSUpdateResponse = NULL;
    PDNS_ZONE_RECORD pDNSZoneRecord = NULL;
    PDNS_RR_RECORD   pDNSARecord = NULL;
    PDNS_RR_RECORD   pDNSPRRecord = NULL;
    DWORD iAddr = 0;

    LWDNS_LOG_INFO("Attempting DNS Update (in-secure)");

    dwError = DNSUpdateCreateUpdateRequest(
                    &pDNSUpdateRequest);
    BAIL_ON_LWDNS_ERROR(dwError);

    dwError = DNSCreateZoneRecord(
                        pszDomainName,
                        &pDNSZoneRecord);
    BAIL_ON_LWDNS_ERROR(dwError);

    dwError = DNSUpdateAddZoneSection(
                        pDNSUpdateRequest,
                        pDNSZoneRecord);
    BAIL_ON_LWDNS_ERROR(dwError);

    pDNSZoneRecord = NULL;

    dwError = DNSCreateNameNotInUseRecord(
                    pszHostnameFQDN,
                    &pDNSPRRecord);
    BAIL_ON_LWDNS_ERROR(dwError);

    dwError = DNSUpdateAddPRSection(
                    pDNSUpdateRequest,
                    pDNSPRRecord);
    BAIL_ON_LWDNS_ERROR(dwError);

    pDNSPRRecord = NULL;

#if 0
    dwError = DNSCreateNameInUseRecord(
                    pszHostnameFQDN,
                    &pDNSPRRecord);
    BAIL_ON_LWDNS_ERROR(dwError);

    dwError = DNSUpdateAddPRSection(
                    pDNSUpdateRequest,
                    pDNSPRRecord);
    BAIL_ON_LWDNS_ERROR(dwError);

    pDNSPRRecord = NULL;

#endif

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

    if (pDNSPRRecord)
    {
        DNSFreeRecord(pDNSPRRecord);
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
    PCSTR pszDomainName,
    PCSTR pszHostnameFQDN,
    PSOCKADDR_IN pAddrArray,
    DWORD  dwNumAddrs,
    PDNS_UPDATE_RESPONSE * ppDNSUpdateResponse
    )
{
    DWORD dwError = 0;
    PDNS_UPDATE_REQUEST pDNSUpdateRequest = NULL;
    PDNS_UPDATE_RESPONSE pDNSUpdateResponse = NULL;
    PDNS_ZONE_RECORD pDNSZoneRecord = NULL;
    PDNS_RR_RECORD pDNSPRRecord = NULL;
    PDNS_RR_RECORD pDNSARecord = NULL;
    DWORD iAddr = 0;

    LWDNS_LOG_INFO("Attempting DNS Update (secure)");

    dwError = DNSUpdateCreateUpdateRequest(
                    &pDNSUpdateRequest);
    BAIL_ON_LWDNS_ERROR(dwError);

    dwError = DNSCreateZoneRecord(
                        pszDomainName,
                        &pDNSZoneRecord);
    BAIL_ON_LWDNS_ERROR(dwError);

    dwError = DNSUpdateAddZoneSection(
                        pDNSUpdateRequest,
                        pDNSZoneRecord);
    BAIL_ON_LWDNS_ERROR(dwError);

    pDNSZoneRecord = NULL;

    dwError = DNSCreateNameNotInUseRecord(
                    pszHostnameFQDN,
                    &pDNSPRRecord);
    BAIL_ON_LWDNS_ERROR(dwError);

    dwError = DNSUpdateAddPRSection(
                    pDNSUpdateRequest,
                    pDNSPRRecord);
    BAIL_ON_LWDNS_ERROR(dwError);

    pDNSPRRecord = NULL;

#if 0
    dwError = DNSCreateNameInUseRecord(
                    pszDomainName,
                    &pDNSPRRecord);
    BAIL_ON_LWDNS_ERROR(dwError);

    dwError = DNSUpdateAddPRSection(
                    pDNSUpdateRequest,
                    pDNSPRRecord);
    BAIL_ON_LWDNS_ERROR(dwError);

    pDNSPRRecord = NULL;
#endif

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

