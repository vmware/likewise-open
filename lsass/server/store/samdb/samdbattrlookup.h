#ifndef __SAM_DB_ATTR_LOOKUP_H__
#define __SAM_DB_ATTR_LOOKUP_H__

DWORD
SamDbAttributeLookupInitContents(
    PSAM_DB_ATTR_LOOKUP   pAttrLookup,
    PSAM_DB_ATTRIBUTE_MAP pAttrMap,
    DWORD                 dwNumMaps
    );

VOID
SamDbAttributeLookupFreeContents(
    PSAM_DB_ATTR_LOOKUP pAttrLookup
    );

#endif /* __SAM_DB_ATTR_LOOKUP_H__ */
