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
