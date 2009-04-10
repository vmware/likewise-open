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

typedef DWORD SAMDB_ACB, *PSAMDB_ACB;

#define SAMDB_ACB_DISABLED                 (0x00000001)
#define SAMDB_ACB_HOMDIRREQ                (0x00000002)
#define SAMDB_ACB_PWNOTREQ                 (0x00000004)
#define SAMDB_ACB_TEMPDUP                  (0x00000008)
#define SAMDB_ACB_NORMAL                   (0x00000010)
#define SAMDB_ACB_MNS                      (0x00000020)
#define SAMDB_ACB_DOMTRUST                 (0x00000040)
#define SAMDB_ACB_WSTRUST                  (0x00000080)
#define SAMDB_ACB_SVRTRUST                 (0x00000100)
#define SAMDB_ACB_PWNOEXP                  (0x00000200)
#define SAMDB_ACB_AUTOLOCK                 (0x00000400)
#define SAMDB_ACB_ENC_TXT_PWD_ALLOWED      (0x00000800)
#define SAMDB_ACB_SMARTCARD_REQUIRED       (0x00001000)
#define SAMDB_ACB_TRUSTED_FOR_DELEGATION   (0x00002000)
#define SAMDB_ACB_NOT_DELEGATED            (0x00004000)
#define SAMDB_ACB_USE_DES_KEY_ONLY         (0x00008000)
#define SAMDB_ACB_DONT_REQUIRE_PREAUTH     (0x00010000)
#define SAMDB_ACB_PW_EXPIRED               (0x00020000)
#define SAMDB_ACB_NO_AUTH_DATA_REQD        (0x00080000)


#endif /* __SAMDBDEFS_H__ */


/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
