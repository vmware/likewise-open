#ifndef _SAMRSRVDEFS_H_
#define _SAMRSRVDEFS_H_

#ifdef _DCE_IDL_
typedef [context_handle] void* CONNECT_HANDLE;
#else
typedef void* CONNECT_HANDLE;
#endif


#define BAIL_ON_NTSTATUS_ERROR(status)                   \
    do {                                                 \
        if ((status) != STATUS_SUCCESS) {                \
            LSA_LOG_ERROR("Error: NTSTATUS = 0x%08x",    \
                          (status));                     \
            goto error;                                  \
        }                                                \
    } while (0)



#define BAIL_ON_NO_MEMORY(ptr)                           \
    do {                                                 \
        if ((ptr) == NULL) {                             \
            status = STATUS_NO_MEMORY;                   \
            LSA_LOG_ERROR("Error: out of memory");       \
            goto error;                                  \
        }                                                \
    } while (0)


#ifdef BAIL_ON_LSA_ERROR
#undef BAIL_ON_LSA_ERROR
#endif


#define BAIL_ON_LSA_ERROR(err)                           \
    do {                                                 \
        if ((err) != 0) {                                \
            switch ((err)) {                             \
            case LSA_ERROR_SAM_DATABASE_ERROR:           \
                status = STATUS_SAM_INIT_FAILURE;        \
                break;                                   \
                                                         \
            default:                                     \
                status = STATUS_UNSUCCESSFUL;            \
            }                                            \
                                                         \
            LSA_LOG_ERROR("Error at %s:%d [code: %d]",   \
                          __FILE__, __LINE__, (err));    \
            goto error;                                  \
        }                                                \
    } while (0)


#define GLOBAL_DATA_LOCK(locked)                         \
    do {                                                 \
        int ret = 0;                                     \
        ret = pthread_mutex_lock(&gSamrSrvDataMutex);    \
        if (ret) {                                       \
            status = STATUS_UNSUCCESSFUL;		         \
            goto error;                                  \
                                                         \
        } else {                                         \
            (locked) = 1;                                \
        }                                                \
    } while (0)


#define GLOBAL_DATA_UNLOCK(locked)                       \
    do {                                                 \
        int ret = 0;                                     \
        if (!locked) break;                              \
        ret = pthread_mutex_unlock(&gSamrSrvDataMutex);  \
        if (ret && status == STATUS_SUCCESS) {           \
            status = STATUS_UNSUCCESSFUL;                \
                                                         \
        } else {                                         \
            (locked) = 0;                                \
        }                                                \
    } while (0)



#endif /* _SAMRSRVDEFS_H_ */


/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
