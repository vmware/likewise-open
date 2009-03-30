#ifndef __SAMDBDEFS_H__
#define __SAMDBDEFS_H__

#define SAM_DB_DIR CACHEDIR   "/db"
#define SAM_DB     SAM_DB_DIR "/sam.db"

#define SAMDB_LOG_ERROR(pszFormat, ...)
#define SAMDB_LOG_WARNING(pszFormat, ...)
#define SAMDB_LOG_INFO(pszFormat, ...)
#define SAMDB_LOG_VERBOSE(pszFormat, ...)
#define SAMDB_LOG_DEBUG(pszFormat, ...)

#define BAIL_ON_SAMDB_ERROR(dwError) \
    if (dwError) goto error;

#define SAMDB_LOCK_MUTEX(bInLock, mutex) \
    if (!bInLock) { \
       int thr_err = pthread_mutex_lock(mutex); \
       if (thr_err) { \
           SAMDB_LOG_ERROR("Failed to lock mutex. Aborting program"); \
           abort(); \
       } \
       bInLock = TRUE; \
    }

#define SAMDB_UNLOCK_MUTEX(bInLock, mutex) \
    if (bInLock) { \
       int thr_err = pthread_mutex_unlock(mutex); \
       if (thr_err) { \
           SAMDB_LOG_ERROR("Failed to unlock mutex. Aborting program"); \
           abort(); \
       } \
       bInLock = FALSE; \
    }

#define SAMDB_LOCK_RWMUTEX_SHARED(bInLock, mutex) \
    if (!bInLock) { \
       int thr_err = pthread_rwlock_rdlock(mutex); \
       if (thr_err) { \
           SAMDB_LOG_ERROR("Failed to acquire shared lock on rw mutex. Aborting program"); \
           abort(); \
       } \
       bInLock = TRUE; \
    }

#define SAMDB_LOCK_RWMUTEX_EXCLUSIVE(bInLock, mutex) \
    if (!bInLock) { \
       int thr_err = pthread_rwlock_wrlock(mutex); \
       if (thr_err) { \
           SAMDB_LOG_ERROR("Failed to acquire exclusive lock on rw mutex. Aborting program"); \
           abort(); \
       } \
       bInLock = TRUE; \
    }

#define SAMDB_UNLOCK_RWMUTEX(bInLock, mutex) \
    if (bInLock) { \
       int thr_err = pthread_rwlock_unlock(mutex); \
       if (thr_err) { \
           SAMDB_LOG_ERROR("Failed to unlock rw mutex. Aborting program"); \
           abort(); \
       } \
       bInLock = FALSE; \
    }

typedef enum
{
    SAMDB_ENTRY_TYPE_UNKNOWN = 0,
    SAMDB_ENTRY_TYPE_USER_OR_GROUP,
    SAMDB_ENTRY_TYPE_DOMAIN

} SAMDB_ENTRY_TYPE, *PSAMDB_ENTRY_TYPE;

#define ATTR_IS_MANDATORY     TRUE
#define ATTR_IS_NOT_MANDATORY FALSE
#define ATTR_IS_MUTABLE       TRUE
#define ATTR_IS_IMMUTABLE     FALSE

typedef enum
{
    SAMDB_DN_TOKEN_TYPE_UNKNOWN = 0,
    SAMDB_DN_TOKEN_TYPE_DC,
    SAMDB_DN_TOKEN_TYPE_CN,
    SAMDB_DN_TOKEN_TYPE_OU

} SAMDB_DN_TOKEN_TYPE;

#endif /* __SAMDBDEFS_H__ */


/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
