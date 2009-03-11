#ifndef __SAMDBSTRUCTS_H__
#define __SAMDBSTRUCTS_H__

typedef struct _SAMDB_INTERLOCKED_COUNTER
{
    pthread_mutex_t  mutex;
    pthread_mutex_t* pMutex;
    DWORD            dwCounter;

} SAMDB_INTERLOCKED_COUNTER, *PSAMDB_INTERLOCKED_COUNTER;

typedef struct _SAM_DB_INSTANCE_LOCK
{
    SAMDB_INTERLOCKED_COUNTER counter;

    pthread_rwlock_t  rwLock;
    pthread_rwlock_t* pRwLock;

} SAM_DB_INSTANCE_LOCK, *PSAM_DB_INSTANCE_LOCK;

typedef struct _SAM_DB_CONTEXT
{
    PSAM_DB_INSTANCE_LOCK pDbInstanceLock;

    sqlite3* pDbHandle;

} SAM_DB_CONTEXT, *PSAM_DB_CONTEXT;

typedef struct _SAM_DIRECTORY_CONTEXT
{
    pthread_rwlock_t  rwLock;
    pthread_rwlock_t* pRwLock;

    PWSTR    pwszDistinguishedName;
    PWSTR    pwszCredential;
    ULONG    ulMethod;

    PSAM_DB_CONTEXT pDbContext;

} SAM_DIRECTORY_CONTEXT, *PSAM_DIRECTORY_CONTEXT;

typedef struct _SAM_GLOBALS
{
    pthread_mutex_t mutex;

    PSTR            pszProviderName;

    DIRECTORY_PROVIDER_FUNCTION_TABLE providerFunctionTable;

    PSAM_DB_INSTANCE_LOCK pDbInstanceLock;

} SAM_GLOBALS, *PSAM_GLOBALS;

#endif /* __SAMDBSTRUCTS_H__ */

