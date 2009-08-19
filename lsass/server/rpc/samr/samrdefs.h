#ifndef _SAMRSRVDEFS_H_
#define _SAMRSRVDEFS_H_

#ifdef _DCE_IDL_
typedef [context_handle] void* CONNECT_HANDLE;
#else
typedef void* CONNECT_HANDLE;
#endif

#ifdef _DCE_IDL_
typedef [context_handle] void* DOMAIN_HANDLE;
#else
typedef void* DOMAIN_HANDLE;
#endif

#ifdef _DCE_IDL_
typedef [context_handle] void* ACCOUNT_HANDLE;
#else
typedef void* ACCOUNT_HANDLE;
#endif


#define LSA_CFG_TAG_RPC_SERVER                 "rpc server:"
#define LSA_CFG_TAG_SAMR_RPC_SERVER            "samr"

#define LSA_RPC_DIR                            CACHEDIR "/rpc"
#define LSA_DEFAULT_LPC_SOCKET_PATH            LSA_RPC_DIR "/lsass"


#define DS_ATTR_RECORD_ID \
    {'O','b','j','e','c','t','R','e','c','o','r','d','I','d',0}
#define DS_ATTR_DISTINGUISHED_NAME  \
    {'D','i','s','t','i','n','g','u','i','s','h','e','d','N','a','m','e',0}
#define DS_ATTR_OBJECT_CLASS \
    {'O','b','j','e','c','t','C','l','a','s','s',0}
#define DS_ATTR_OBJECT_SID \
    {'O','b','j','e','c','t','S','I','D',0}
#define DS_ATTR_COMMON_NAME \
    {'C','o','m','m','o','n','N','a','m','e',0}
#define DS_ATTR_DOMAIN \
    {'D','o','m','a','i','n',0}
#define DS_ATTR_NETBIOS_NAME \
    {'N','e','t','B','I','O','S','N','a','m','e',0}
#define DS_ATTR_COMMON_NAME \
    {'C','o','m','m','o','n','N','a','m','e',0}
#define DS_ATTR_SAM_ACCOUNT_NAME \
    {'S','a','m','A','c','c','o','u','n','t','N','a','m','e',0}
#define DS_ATTR_UID \
    {'U','I','D',0}
#define DS_ATTR_GID \
    {'G','I','D',0}
#define DS_ATTR_PRIMARY_GROUP \
    {'P','r','i','m','a','r','y','G','r','o','u','p',0}
#define DS_ATTR_FULL_NAME \
    {'F','u','l','l','N','a','m','e',0}
#define DS_ATTR_DESCRIPTION \
    {'D','e','s','c','r','i','p','t','i','o','n',0}
#define DS_ATTR_COMMENT \
    {'C','o','m','m','e','n','t',0}
#define DS_ATTR_HOME_DIR \
    {'H','o','m','e','d','i','r',0}
#define DS_ATTR_HOME_DRIVE \
    {'H','o','m','e','d','r','i','v','e',0}
#define DS_ATTR_LOGON_SCRIPT \
    {'L','o','g','o','n','S','c','r','i','p','t',0}
#define DS_ATTR_PROFILE_PATH \
    {'P','r','o','f','i','l','e','P','a','t','h',0}
#define DS_ATTR_WORKSTATIONS \
    {'W','o','r','k','s','t','a','t','i','o','n','s',0}
#define DS_ATTR_PARAMETERS \
    {'P','a','r','a','m','e','t','e','r','s',0}
#define DS_ATTR_PASSWORD_LAST_SET \
    {'P','a','s','s','w','o','r','d','L','a','s','t','S','e','t',0}
#define DS_ATTR_ALLOW_PASSWORD_CHANGE \
    {'A','l','l','o','w','P','a','s','s','w','o','r','d','C','h','a','n','g','e',0}
#define DS_ATTR_FORCE_PASSWORD_CHANGE \
    {'F','o','r','c','e','P','a','s','s','w','o','r','d','C','h','a','n','g','e',0}
#define DS_ATTR_ACCOUNT_FLAGS \
    {'A','c','c','o','u','n','t','F','l','a','g','s',0}
#define DS_ATTR_CREATED_TIME \
    {'C','r','e','a','t','e','d','T','i','m','e',0}
#define DS_ATTR_MIN_PWD_LENGTH \
    {'M','i','n','P','w','d','L','e','n','g','t','h',0}
#define DS_ATTR_PWD_HISTORY_LENGTH \
    {'P','w','d','H','i','s','t','o','r','y','L','e','n','g','t','h',0}
#define DS_ATTR_PWD_PROPERTIES \
    {'P','w','d','P','r','o','p','e','r','t','i','e','s',0}
#define DS_ATTR_MAX_PWD_AGE \
    {'M','a','x','P','w','d','A','g','e',0}
#define DS_ATTR_MIN_PWD_AGE \
    {'M','i','n','P','w','d','A','g','e',0}
#define DS_ATTR_LAST_LOGON \
    {'L','a','s','t','L','o','g','o','n',0}
#define DS_ATTR_LAST_LOGOFF \
    {'L','a','s','t','L','o','g','o','f','f',0}
#define DS_ATTR_FORCE_LOGOFF_TIME \
    {'F','o','r','c','e','L','o','g','o','f','f','T','i','m','e',0}
#define DS_ATTR_LOCKOUT_DURATION \
    {'L','o','c','k','o','u','t','D','u','r','a','t','i','o','n',0}
#define DS_ATTR_LOCKOUT_WINDOW \
    {'L','o','c','k','o','u','t','W','i','n','d','o','w',0}
#define DS_ATTR_LOCKOUT_THRESHOLD \
    {'L','o','c','k','o','u','t','T','h','r','e','s','h','o','l','d',0}
#define DS_ATTR_LOGON_COUNT \
    {'L','o','g','o','n','C','o','u','n','t',0}
#define DS_ATTR_BAD_PASSWORD_COUNT \
    {'B','a','d','P','w','d','C','o','u','n','t',0}
#define DS_ATTR_LOGON_HOURS \
    {'L','o','g','o','n','H','o','u','r','s',0}
#define DS_ATTR_ACCOUNT_EXPIRY \
    {'A','c','c','o','u','n','t','E','x','p','i','r','y',0}
#define DS_ATTR_ROLE \
    {'R','o','l','e',0}
#define DS_ATTR_COUNTRY_CODE \
    {'C','o','u','n','t','r','y','C','o','d','e',0}
#define DS_ATTR_CODE_PAGE \
    {'C','o','d','e','P','a','g','e',0}
#define DS_ATTR_LM_HASH \
    {'L','M','H','a','s','h',0}
#define DS_ATTR_NT_HASH \
    {'N','T','H','a','s','h',0}


#define DS_OBJECT_CLASS_DOMAIN           (1)
#define DS_OBJECT_CLASS_BUILTIN_DOMAIN   (2)
#define DS_OBJECT_CLASS_GROUP            (4)
#define DS_OBJECT_CLASS_USER             (5)


#define SAMR_BUILTIN_DOMAIN_NAME \
    {'B','U','I','L','T','I','N',0}


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


#define BAIL_ON_INVALID_PARAMETER(cond)                  \
    do {                                                 \
        if (!(cond)) {                                   \
            status = STATUS_INVALID_PARAMETER;           \
            LSA_LOG_ERROR("Error: invalid parameter");   \
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
            case LW_ERROR_SAM_DATABASE_ERROR:           \
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
