#ifndef __SAMDBDN_H__
#define __SAMDBDN_H__

DWORD
SamDbParseDN(
    PWSTR             pwszObjectDN,
    PWSTR*            ppwszObjectName,
    PWSTR*            ppwszDomain,
    PSAMDB_ENTRY_TYPE pEntryType
    );

#endif /* __SAMDBDN_H__ */

