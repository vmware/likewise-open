DWORD
VmDirGetDefaultSearchBase(
        PCSTR pszBindDN,
        PSTR* ppszSearchBase
        );

DWORD
VmDirAttributesFromWc16Attributes(
    PWSTR *wszAttributes,
    PSTR **pppszAttributes
    );

DWORD
VmDirAttributesFree(
    PSTR **pppszAttributes
    );

DWORD
VmDirAttributesWc16FromCAttributes(
    PSTR *pszAttributes,
    PWSTR **pppwszAttributes
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
