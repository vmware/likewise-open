
DWORD
DNSUpdateCreateARUpdateRequest(
    PDNS_UPDATE_REQUEST* ppDNSUpdateRequest,
    PCSTR pszZoneName,
    PCSTR pszHostnameFQDN,
    DWORD  dwNumAddrs,
    PSOCKADDR_IN pAddrArray
    );

DWORD
DNSNegotiateContextAndSecureUpdate(
    HANDLE hDNSServer,
    PCSTR  pszServiceName,
    PCSTR  pszDomainName,
    PCSTR  pszHost,
    DWORD  dwIPAddress
    );



DWORD
DNSSendUpdate(
    HANDLE hDNSServer,
    PCSTR  pszZoneName,
    PCSTR  pszHost,
    DWORD  dwNumAddrs,
    PSOCKADDR_IN pAddrArray,
    PDNS_UPDATE_RESPONSE * ppDNSUpdateResponse
    );

DWORD
DNSSendSecureUpdate(
    HANDLE hDNSServer,
    PCtxtHandle pGSSContext,
    PCSTR pszKeyName,
    PCSTR pszZoneName,
    PCSTR pszHost,
    DWORD  dwNumAddrs,
    PSOCKADDR_IN pAddrArray,
    PDNS_UPDATE_RESPONSE * ppDNSUpdateResponse
    );


DWORD
DNSUpdateGenerateSignature(
    PCtxtHandle pGSSContext,
    PDNS_UPDATE_REQUEST pDNSUpdateRequest,
    PCSTR pszKeyName
    );

DWORD
DNSBuildMessageBuffer(
    PDNS_UPDATE_REQUEST pDNSUpdateRequest,
    PCSTR pszKeyName,
    DWORD * pdwTimeSigned,
    WORD * pwFudge,
    PBYTE * ppMessageBuffer,
    PDWORD pdwMessageSize
    );

