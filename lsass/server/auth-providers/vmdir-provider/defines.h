/*
 * Copyright (C) VMware. All rights reserved.
 */

#define LSA_PROVIDER_TAG_VMDIR "lsa-vmdir-provider"

//
// Registry keys
//
#define VMDIR_REG_KEY "Services\\vmdir"

#define VMDIR_AUTH_PROVIDER_KEY "Services\\lsass\\Parameters\\Providers\\VmDir"

#define VMDIR_REG_KEY_BIND_PROTOCOL       "BindProtocol"
#define VMDIR_BIND_PROTOCOL_KERBEROS_STR  "kerberos"
#define VMDIR_BIND_PROTOCOL_SRP_STR       "srp"

#define VMDIR_REG_KEY_CACHE_ENTRY_EXPIRY  "CacheEntryExpiry"
#define VMDIR_CACHE_ENTRY_EXPIRY_DEFAULT_SECS   (4 * LSA_SECONDS_IN_HOUR)

#define VMDIR_REG_KEY_BIND_INFO_ACCOUNT   "dcAccount"
#define VMDIR_REG_KEY_BIND_INFO_BIND_DN   "dcAccountDN"
#define VMDIR_REG_KEY_BIND_INFO_PASSWORD  "dcAccountPassword"

#define VMAFD_REG_KEY "Services\\vmafd\\Parameters"

#define VMAFD_REG_KEY_DOMAIN_STATE "DomainState"
#define VMAFD_REG_KEY_DOMAIN_NAME  "DomainName"
#define VMAFD_REG_KEY_DC_NAME      "DCName"

#define BAIL_ON_VMDIR_ERROR(dwError) \
        if (dwError != LW_ERROR_SUCCESS) \
        { \
            goto error; \
        }

#define BAIL_ON_KRB_ERROR(ctx, ret) \
    do { \
        if (ret) \
        { \
           (dwError) = LwTranslateKrb5Error(ctx, ret, __FUNCTION__, __FILE__, __LINE__); \
           goto error; \
        } \
    } while (0)

#define BAIL_ON_ERRNO_ERROR(ret) \
    do { \
        if (ret) \
        { \
           (dwError) = LwErrnoToWin32Error(ret); \
           goto error; \
        } \
    } while (0)

#define VMDIR_ACQUIRE_RWLOCK_SHARED(pRWLock, bLocked) \
    VmDirRWLockAcquire(pRWLock, FALSE, &bLocked)

#define VMDIR_ACQUIRE_RWLOCK_EXCLUSIVE(pRWLock, bLocked) \
    VmDirRWLockAcquire(pRWLock, TRUE, &bLocked)

#define VMDIR_RELEASE_RWLOCK(pRWLock, bLocked) \
    VmDirRWLockRelease(pRWLock, &bLocked)

#define LOG_FUNC_ENTER LSA_LOG_INFO("Function enter (%s)", __FUNCTION__)
#define LOG_FUNC_EXIT  LSA_LOG_INFO("Function exit  (%s)", __FUNCTION__)

#ifndef IsNullOrEmptyString
#define IsNullOrEmptyString(str) (!(str) || !(*str))
#endif

#define VMDIR_USER_SHELL "/bin/bash"

#define DEFAULT_LDAP_QUERY_TIMEOUT_SECS  15

#define VMDIR_OBJ_CLASS_USER             "user"
#define VMDIR_OBJ_CLASS_COMPUTER         "computer"
#define VMDIR_OBJ_CLASS_GROUP            "group"

#define VMDIR_ATTR_NAME_ACCOUNT          "sAMAccountName"
#define VMDIR_ATTR_NAME_DN               "distinguishedName"
#define VMDIR_ATTR_NAME_OBJECTSID        "objectSid"
// #define VMDIR_ATTR_NAME_UID              "uidNumber"
// #define VMDIR_ATTR_NAME_GID              "gidNumber"
// #define VMDIR_ATTR_NAME_PRIMARY_GID      "primaryGroupID"
#define VMDIR_ATTR_NAME_UAC              "userAccountControl"
// #define VMDIR_ATTR_NAME_GECOS            "gecos"
#define VMDIR_ATTR_NAME_FIRST_NAME       "givenName"
// #define VMDIR_ATTR_NAME_MIDDLE_NAME      "middleName"
#define VMDIR_ATTR_NAME_LAST_NAME        "sn"
// #define VMDIR_ATTR_NAME_DISPLAY_NAME     "displayName"
// #define VMDIR_ATTR_NAME_MAX_PASSWORD_AGE "maxPwdAge"
// #define VMDIR_ATTR_NAME_LOGIN_SHELL      "loginShell"
// #define VMDIR_ATTR_NAME_HOME_DIRECTORY   "homeDirectory"
// #define VMDIR_ATTR_NAME_ACCT_EXPIRES     "accountExpires"
#define VMDIR_ATTR_NAME_UPN              "userPrincipalName"
#define VMDIR_ATTR_NAME_MEMBER           "member"
#define VMDIR_ATTR_NAME_MEMBER_OF        "memberOf"
#define VMDIR_ATTR_USER_PASSWORD         "userPassword"
#define VMDIR_ATTR_NAME_USN_CHANGED      "uSNChanged"

#define VMDIR_UAC_FLAG_ACCT_DISABLED     0x00000002
#define VMDIR_UAC_FLAG_ACCT_LOCKED       0x00000010
#define VMDIR_UAC_FLAG_ALLOW_PWD_CHANGE  0x00000040
#define VMDIR_UAC_FLAG_PWD_EXPIRED       0x00800000
#define VMDIR_UAC_FLAG_PWD_NEVER_EXPIRES 0x00010000

#define VMDIR_DEFAULT_PRIMARY_GROUP_NAME "Users"

#define VMDIR_KRB5_CC_NAME "FILE:/var/lib/likewise/krb5cc_vmdir_provider"
