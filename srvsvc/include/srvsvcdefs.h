#ifndef __SRVSVC_DEFS_H__
#define __SRVSVC_DEFS_H__

#ifndef _WIN32

#define PATH_SEPARATOR_STR "/"

#else

#define PATH_SEPARATOR_STR "\\"

#endif

#ifndef HOST_NAME_MAX
#define HOST_NAME_MAX 255
#endif

/*
 * Memory related macros
 */

#define SRVSVC_SAFE_FREE(ptr) \
            do { if (ptr) free(ptr); (ptr) = NULL; } while (0)

/*
 * Error check macros
 */

#define BAIL_ON_SRVSVC_ERROR(dwError)                                   \
    if (dwError) {                                                      \
        SRVSVC_LOG_DEBUG("Error at %s:%d. Error [code:%d]",             \
                         __FILE__, __LINE__, dwError);                  \
        goto error;                                                     \
    }

#define BAIL_ON_NT_STATUS(ntStatus)                                     \
    if ((ntStatus)) {                                                   \
       SRVSVC_LOG_DEBUG("Error at %s:%d [status: %s = 0x%08X (%d)]",    \
                     __FILE__,                                          \
                     __LINE__,                                          \
                     LwNtStatusToName(ntStatus),                        \
                     ntStatus, ntStatus);                               \
       goto error;                                                      \
    }

#define BAIL_ON_WIN_ERROR(err)                                          \
    if ((err) != ERROR_SUCCESS) {                                       \
        SRVSVC_LOG_DEBUG("Error at %s:%d. Error [code:%d]",             \
                     __FILE__,                                          \
                     __LINE__,                                          \
                     err);                                              \
        goto error;                                                     \
    }

#define BAIL_ON_RPC_STATUS(st)    \
    if ((st) != RPC_S_OK) {       \
        goto error;               \
    }

#define BAIL_ON_NO_MEMORY_RPCSTATUS(p, status)  \
    if ((p) == NULL) {                          \
        status = RPC_S_OUT_OF_MEMORY;           \
        goto error;                             \
    }

#define BAIL_ON_INVALID_PTR_RPCSTATUS(p, status)    \
    if ((p) == NULL) {                              \
        status = RPC_S_INVALID_ARG;                 \
        goto error;                                 \
    }

#define BAIL_ON_NULL_PTR(p, status)              \
    if ((p) == NULL) {                           \
        status = ERROR_OUTOFMEMORY;              \
        goto error;                              \
    }

#define BAIL_ON_INVALID_PTR(p, status)           \
    if ((p) == NULL) {                           \
        status = ERROR_INVALID_PARAMETER;        \
        goto error;                              \
    }

#define BAIL_ON_DCE_ERROR(dwError, rpcstatus)                           \
    if ((rpcstatus) != RPC_S_OK)                                        \
    {                                                                   \
        dce_error_string_t errstr;                                      \
        int error_status;                                               \
        dce_error_inq_text((rpcstatus), (unsigned char*)errstr,         \
                           &error_status);                              \
        if (error_status == error_status_ok)                            \
        {                                                               \
            SRVSVC_LOG_ERROR("DCE Error [0x%8lx] Reason [%s]",          \
                             (unsigned long)(rpcstatus), errstr);       \
        }                                                               \
        else                                                            \
        {                                                               \
            SRVSVC_LOG_ERROR("DCE Error [0x%8lx]",                      \
                             (unsigned long)(rpcstatus));               \
        }                                                               \
                                                                        \
        switch ((rpcstatus)) {                                          \
        case RPC_S_INVALID_STRING_BINDING:                              \
            (dwError) = SRVSVC_ERROR_RPC_EXCEPTION_UPON_RPC_BINDING;    \
            break;                                                      \
                                                                        \
        default:                                                        \
            (dwError) = SRVSVC_ERROR_RPC_EXCEPTION;                     \
        }                                                               \
                                                                        \
        goto error;                                                     \
    }

#define SRVSVC_LOCK_MUTEX(bInLock, pMutex)           \
    if (!bInLock) {                                  \
        int thr_err = pthread_mutex_lock(pMutex);    \
        if (thr_err) {                               \
            abort();                                 \
        }                                            \
        bInLock = TRUE;                              \
    }

#define SRVSVC_UNLOCK_MUTEX(bInLock, pMutex)        \
    if (bInLock) {                                   \
        int thr_err = pthread_mutex_unlock(pMutex);  \
        if (thr_err) {                               \
            abort();                                 \
        }                                            \
        bInLock = FALSE;                             \
    }

#define SRVSVC_LOCK_RWMUTEX_SHARED(bInLock, pMutex) \
    if (!bInLock) {                                 \
        int thr_err = pthread_rwlock_rdlock(mutex); \
        if (thr_err) {                              \
            abort();                                \
        }                                           \
        bInLock = TRUE;                             \
    }

#define SRVSVC_LOCK_RWMUTEX_EXCLUSIVE(bInLock, pMutex)  \
    if (!bInLock) {                                     \
        int thr_err = pthread_rwlock_wrlock(pMutex);    \
        if (thr_err) {                                  \
            abort();                                    \
        }                                               \
        bInLock = TRUE;                                 \
    }

#define SRVSVC_UNLOCK_RWMUTEX(bInLock, pMutex)       \
    if (bInLock) {                                   \
        int thr_err = pthread_rwlock_unlock(pMutex); \
        if (thr_err) {                               \
            abort();                                 \
        }                                            \
        bInLock = FALSE;                             \
    }

#endif /* __SRVSVC_DEFS_H__ */
