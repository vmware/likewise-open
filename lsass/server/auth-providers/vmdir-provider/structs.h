/*
 * Copyright (C) VMware. All rights reserved.
 */

typedef enum
{
    VMDIR_ATTR_TYPE_UNKNOWN = 0,
    VMDIR_ATTR_TYPE_INT32,
    VMDIR_ATTR_TYPE_UINT32,
    VMDIR_ATTR_TYPE_INT64,
    VMDIR_ATTR_TYPE_UINT64,
    VMDIR_ATTR_TYPE_STRING,
    VMDIR_ATTR_TYPE_MULTI_STRING,
    VMDIR_ATTR_TYPE_DN,
    VMDIR_ATTR_TYPE_BINARY

} VMDIR_ATTR_TYPE;

typedef struct _VMDIR_ATTR
{
    PCSTR           pszName;

    VMDIR_ATTR_TYPE type;

    union
    {
        PINT32   pData_int32;
        PUINT32  pData_uint32;
        PINT64   pData_int64;
        PUINT64  pData_uint64;
        PSTR*    ppszData;
        PSTR**   pppszStrArray;
        PBYTE*   ppData;
    } dataRef;

    size_t  size;

    PDWORD  pdwCount;

    BOOLEAN bOptional;

} VMDIR_ATTR, *PVMDIR_ATTR;

typedef struct _VMDIR_BIND_INFO
{
    LONG refCount;

    PSTR pszURI;
    PSTR pszUPN;
    PSTR pszDomainFqdn;
    PSTR pszDomainShort;
    PSTR pszSearchBase;

} VMDIR_BIND_INFO, *PVMDIR_BIND_INFO;

typedef struct _VMDIR_SASL_INFO
{
    PCSTR pszRealm;
    PCSTR pszAuthName;
    PCSTR pszUser;
    PCSTR pszPassword;
} VMDIR_SASL_INFO, *PVMDIR_SASL_INFO;

typedef struct _MEM_CACHE_CONNECTION LSA_CACHE_HANDLE, *PLSA_CACHE_HANDLE;

typedef struct _VMDIR_DIR_CONTEXT
{
    PVMDIR_BIND_INFO pBindInfo;
    LDAP*            pLd;
    DWORD            dwCacheEntryExpiry;

} VMDIR_DIR_CONTEXT, *PVMDIR_DIR_CONTEXT;

typedef struct _VMDIR_AUTH_PROVIDER_CONTEXT
{
    pthread_mutex_t  mutex;
    pthread_mutex_t* pMutex;

    uid_t peer_uid;
    gid_t peer_gid;
    pid_t peer_pid;

    VMDIR_DIR_CONTEXT dirContext;

} VMDIR_AUTH_PROVIDER_CONTEXT, *PVMDIR_AUTH_PROVIDER_CONTEXT;

typedef enum
{
    VMDIR_ENUM_HANDLE_TYPE_OBJECTS = 0,
    VMDIR_ENUM_HANDLE_TYPE_MEMBERS

} VMDIR_ENUM_HANDLE_TYPE;

typedef struct _VMDIR_ENUM_HANDLE
{
    VMDIR_ENUM_HANDLE_TYPE type;

    PVMDIR_DIR_CONTEXT pDirContext;

    LSA_OBJECT_TYPE    objectType;

    PSTR*              ppszDNArray;
    DWORD              dwDNCount;

    LDAPMessage*       pSearchResult;
    LDAPMessage*       pCurrentEntry;
    LONG64             llLastUSNChanged;

    int                sizeLimit;        // # objects to retrieve per query
    DWORD              dwRemaining;      // objects to be consumed from result
    DWORD              dwIndex;          // current index in result set

} VMDIR_ENUM_HANDLE, *PVMDIR_ENUM_HANDLE;

typedef enum
{
    VMDIR_JOIN_STATE_UNSET,
    VMDIR_JOIN_STATE_NOT_JOINED,
    VMDIR_JOIN_STATE_JOINING,
    VMDIR_JOIN_STATE_JOINED,
} VMDIR_JOIN_STATE;

typedef enum
{
    VMDIR_BIND_PROTOCOL_UNSET,
    VMDIR_BIND_PROTOCOL_KERBEROS,
    VMDIR_BIND_PROTOCOL_SRP,
    VMDIR_BIND_PROTOCOL_SPNEGO
} VMDIR_BIND_PROTOCOL;

typedef enum
{
    VMDIR_REFRESH_STATE_UNSET,
    VMDIR_REFRESH_STATE_SLEEPING,
    VMDIR_REFRESH_STATE_POLLING,
    VMDIR_REFRESH_STATE_CONFIGURING,
    VMDIR_REFRESH_STATE_REFRESHING,
    VMDIR_REFRESH_STATE_WAITING,
    VMDIR_REFRESH_STATE_STOPPING,
} VMDIR_REFRESH_STATE;

typedef struct _VMDIR_REFRESH_CONTEXT {
    pthread_mutex_t mutex;
    pthread_mutex_t *pMutex;
    pthread_cond_t cond;
    pthread_cond_t *pCond;
    pthread_t thread;
    pthread_t *pThread;
    pthread_rwlock_t rwlock;
    pthread_rwlock_t *pRwlock;
    VMDIR_REFRESH_STATE state;
} VMDIR_REFRESH_CONTEXT, *PVMDIR_REFRESH_CONTEXT;

struct _LSA_DB_CONNECTION;
typedef struct _LSA_DB_CONNECTION *LSA_DB_HANDLE;
typedef LSA_DB_HANDLE *PLSA_DB_HANDLE;

typedef struct _VMDIR_AUTH_PROVIDER_GLOBALS
{
    pthread_rwlock_t   mutex_rw;
    pthread_rwlock_t*  pMutex_rw;
    pthread_mutex_t    mutex;
    pthread_mutex_t*   pMutex;

    PVMDIR_BIND_INFO   pBindInfo;
    VMDIR_JOIN_STATE   joinState;
    PVMDIR_REFRESH_CONTEXT pRefreshContext;
    PLSA_CACHE_HANDLE pCacheHandle;

    LSA_DB_HANDLE hDb;
    
    VMDIR_BIND_PROTOCOL bindProtocol;
    DWORD dwCacheEntryExpiry;

} VMDIR_AUTH_PROVIDER_GLOBALS, *PVMDIR_AUTH_PROVIDER_GLOBALS;

typedef struct _LSA_VMDIR_PROVIDER_STATE {
    int nRefCount;
} LSA_VMDIR_PROVIDER_STATE, *PLSA_VMDIR_PROVIDER_STATE;
