#ifndef _SAMRDEFS_H_
#define _SAMRDEFS_H_

#ifdef _DCE_IDL_
typedef [context_handle] void* CONNECT_HANDLE;
#else
typedef void* CONNECT_HANDLE;
#endif


#define BAIL_ON_NTSTATUS_ERROR(status)                   \
    do {                                                 \
        if ((status) != STATUS_SUCCESS) {                \
            dwError = NtStatusToUnixErrno((status));     \
            goto error;                                  \
        }                                                \
    } while (0)



#define BAIL_ON_NO_MEMORY(ptr)                           \
    do {                                                 \
        if ((ptr) == NULL) {                             \
            status = STATUS_NO_MEMORY;                   \
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



#endif /* _SAMRDEFS_H_ */


/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
