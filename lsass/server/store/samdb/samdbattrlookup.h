#ifndef __SAM_DB_ATTR_LOOKUP_H__
#define __SAM_DB_ATTR_LOOKUP_H__

DWORD
SamDbAttributeLookupInitContents(
    PSAM_DB_ATTR_LOOKUP   pAttrLookup,
    PSAM_DB_ATTRIBUTE_MAP pAttrMap,
    DWORD                 dwNumMaps
    );

DWORD
SamDbAttributeLookupByName(
    PSAM_DB_ATTR_LOOKUP    pAttrLookup,
    PWSTR                  pwszAttrName,
    PSAM_DB_ATTRIBUTE_MAP* ppAttrMap
    );

VOID
SamDbAttributeLookupFreeContents(
    PSAM_DB_ATTR_LOOKUP pAttrLookup
    );

#endif /* __SAM_DB_ATTR_LOOKUP_H__ */
