#ifndef __SAMDBDN_H__
#define __SAMDBDN_H__

NTSTATUS
SamDbParseDN(
    PWSTR  pwszObjectDN,
    PWSTR* ppwszDN,
    PDWORD pdwType
    );

#endif /* __SAMDBDN_H__ */

