#ifndef __SAMR_STRUCTS_H__
#define __SAMR_STRUCTS_H__

typedef struct __POLICY_HANDLE
{
    ULONG  ulHandleType;
    uuid_t guid;
} POLICY_HANDLE, *PPOLICY_HANDLE;

typedef struct _PASSWORD_INFO
{
    ULONG dummy;
} PASSWORD_INFO, *PPASSWORD_INFO;

typedef struct _ALIAS_INFO
{
    ULONG dummy;
} ALIAS_INFO, *PALIAS_INFO;

typedef struct _USER_INFO
{
    ULONG dummy;
} USER_INFO, *PUSER_INFO;

typedef struct _DOMAIN_INFO
{
    ULONG dummy;
} DOMAIN_INFO, *PDOMAIN_INFO;

typedef ULONG RidNameArray;
typedef ULONG RidWithAttributeArray;

typedef ULONG SidArray;

typedef ULONG EntryArray;

#endif /* __SAMR_STRUCTS_H__ */

