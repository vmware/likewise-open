DWORD
VmDirGetDefaultSearchBase(
        PCSTR pszBindDN,
        PSTR* ppszSearchBase
        );

DWORD
VmDirAttributesFromWc16Attributes(
    PWSTR *wszAttributes,
    PSTR **pppszAttributes,
    DWORD *pdwAttrCount
    );

DWORD
VmDirAttributesFree(
    PSTR **pppszAttributes
    );

DWORD
VmDirAttributesWc16FromCAttributes(
    PSTR *pszAttributes,
    PWSTR **pppwszAttributes,
    DWORD *pdwAttrCount
    );

DWORD
VmDirAttributesWC16Free(
    PWSTR **pppwszAttributes
    );

DWORD
VmDirGetDNFromFQDN(
    PCSTR pszFqdn,
    PSTR *ppszDn
    );
