#ifndef __SAMDBDN_H__
#define __SAMDBDN_H__

DWORD
SamDbParseDN(
    PWSTR       pwszObjectDN,
    PSAM_DB_DN* ppDN
    );

DWORD
SamDbGetDNComponents(
    PSAM_DB_DN pDN,
    PWSTR*     ppwszObjectName,
    PWSTR*     ppwszParentDN,
    PWSTR*     ppwszDomainName
    );

VOID
SamDbFreeDN(
    PSAM_DB_DN pDN
    );

#endif /* __SAMDBDN_H__ */

