#ifndef __SAMDBSTRUCTS_H__
#define __SAMDBSTRUCTS_H__

typedef struct _SAMDB_OBJECTCLASS_TO_ATTR_MAP_INFO
{
    SAMDB_OBJECT_CLASS        objectClass;
    PSAMDB_ATTRIBUTE_MAP_INFO pAttributeMaps;
    DWORD                     dwNumMaps;

} SAMDB_OBJECTCLASS_TO_ATTR_MAP_INFO, *PSAMDB_OBJECTCLASS_TO_ATTR_MAP_INFO;

typedef struct _SAM_DB_INSTANCE_LOCK
{
    LONG refCount;

    pthread_rwlock_t  rwLock;
    pthread_rwlock_t* pRwLock;

} SAM_DB_INSTANCE_LOCK, *PSAM_DB_INSTANCE_LOCK;

typedef struct _SAM_DB_CONTEXT
{
    PSAM_DB_INSTANCE_LOCK pDbLock;

    sqlite3* pDbHandle;

    sqlite3_stmt* pDelObjectStmt;

} SAM_DB_CONTEXT, *PSAM_DB_CONTEXT;

typedef struct _SAM_DB_ATTR_LOOKUP
{
    PLWRTL_RB_TREE pLookupTable;

} SAM_DB_ATTR_LOOKUP, *PSAM_DB_ATTR_LOOKUP;

typedef struct _SAM_DIRECTORY_CONTEXT
{
    pthread_rwlock_t  rwLock;
    pthread_rwlock_t* pRwLock;

    PWSTR    pwszDistinguishedName;
    PWSTR    pwszCredential;
    ULONG    ulMethod;

    PSAM_DB_CONTEXT         pDbContext;

    PSAMDB_OBJECTCLASS_TO_ATTR_MAP_INFO pObjectClassAttrMaps;
    DWORD                               dwNumObjectClassAttrMaps;
    PSAM_DB_ATTR_LOOKUP   pAttrLookup;

} SAM_DIRECTORY_CONTEXT, *PSAM_DIRECTORY_CONTEXT;

typedef struct _SAM_GLOBALS
{
    pthread_mutex_t mutex;

    PSAMDB_OBJECTCLASS_TO_ATTR_MAP_INFO pObjectClassAttrMaps;
    DWORD                               dwNumObjectClassAttrMaps;

    PSAM_DB_ATTRIBUTE_MAP pAttrMaps;
    DWORD                 dwNumMaps;
    SAM_DB_ATTR_LOOKUP    attrLookup;

    PSTR            pszProviderName;

    DIRECTORY_PROVIDER_FUNCTION_TABLE providerFunctionTable;

    PSAM_DB_INSTANCE_LOCK pDbInstanceLock;

} SAM_GLOBALS, *PSAM_GLOBALS;

typedef struct _SAM_DB_DOMAIN_INFO
{
    ULONG ulDomainRecordId;

    PWSTR pwszDomainName;
    PWSTR pwszNetBIOSName;
    PWSTR pwszDomainSID;

} SAM_DB_DOMAIN_INFO, *PSAM_DB_DOMAIN_INFO;

typedef struct _SAMDB_DN_TOKEN
{
    SAMDB_DN_TOKEN_TYPE tokenType;
    PWSTR               pwszToken;
    DWORD               dwLen;

    struct _SAMDB_DN_TOKEN * pNext;

} SAMDB_DN_TOKEN, *PSAMDB_DN_TOKEN;

typedef struct _SAM_DB_DN
{
    PWSTR pwszDN;

    PSAMDB_DN_TOKEN pTokenList;

} SAM_DB_DN, *PSAM_DB_DN;

#endif /* __SAMDBSTRUCTS_H__ */

