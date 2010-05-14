/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

/*
 * Copyright Likewise Software    2004-2008
 * All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or (at
 * your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * for more details.  You should have received a copy of the GNU General
 * Public License along with this program.  If not, see
 * <http://www.gnu.org/licenses/>.
 *
 * LIKEWISE SOFTWARE MAKES THIS SOFTWARE AVAILABLE UNDER OTHER LICENSING
 * TERMS AS WELL.  IF YOU HAVE ENTERED INTO A SEPARATE LICENSE AGREEMENT
 * WITH LIKEWISE SOFTWARE, THEN YOU MAY ELECT TO USE THE SOFTWARE UNDER THE
 * TERMS OF THAT SOFTWARE LICENSE AGREEMENT INSTEAD OF THE TERMS OF THE GNU
 * GENERAL PUBLIC LICENSE, NOTWITHSTANDING THE ABOVE NOTICE.  IF YOU
 * HAVE QUESTIONS, OR WISH TO REQUEST A COPY OF THE ALTERNATE LICENSING
 * TERMS OFFERED BY LIKEWISE SOFTWARE, PLEASE CONTACT LIKEWISE SOFTWARE AT
 * license@likewisesoftware.com
 */

/*
 * Copyright (C) Likewise Software. All rights reserved.
 *
 * Module Name:
 *
 *        regserver.h
 *
 * Abstract:
 *
 *        Registry
 *
 *        Inter-process communication (Server) API for Users
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 *          Wei Fu (wfu@likewise.com)
 */

#ifndef __REGSERVER_H_
#define __REGSERVER_H_

typedef struct __REG_SRV_API_STATE
{
    uid_t  peerUID;
    gid_t  peerGID;
    PACCESS_TOKEN pToken;
    HANDLE hEventLog;
} REG_SRV_API_STATE, *PREG_SRV_API_STATE;

typedef struct __REG_KEY_CONTEXT
{
    LONG refCount;

    pthread_rwlock_t mutex;
    pthread_rwlock_t* pMutex;

    int64_t qwId;
    PWSTR pwszKeyName;

    int64_t qwSdId;
    PSECURITY_DESCRIPTOR_RELATIVE pSecurityDescriptor;
    ULONG ulSecDescLength;
    BOOLEAN bHasSdInfo;


    PWSTR pwszParentKeyName;

    DWORD dwNumSubKeys;
    DWORD dwNumCacheSubKeys;
    size_t sMaxSubKeyLen;
    PWSTR* ppwszSubKeyNames;
    BOOLEAN bHasSubKeyInfo;

    DWORD dwNumValues;
    DWORD dwNumCacheValues;
    size_t sMaxValueNameLen;
    size_t sMaxValueLen;
    PREG_DATA_TYPE pTypes;
    PWSTR* ppwszValueNames;
    PBYTE* ppValues;
    PDWORD pdwValueLen;
    BOOLEAN bHasValueInfo;

} REG_KEY_CONTEXT, *PREG_KEY_CONTEXT;


typedef struct __REG_KEY_HANDLE
{
	ACCESS_MASK AccessGranted;
	PREG_KEY_CONTEXT pKey;

} REG_KEY_HANDLE, *PREG_KEY_HANDLE;


#define LWREG_LOCK_MUTEX(bInLock, mutex) \
    if (!bInLock) { \
       int thr_err = pthread_mutex_lock(mutex); \
       if (thr_err) { \
           LWREG_LOG_ERROR("Failed to lock mutex. Aborting program"); \
           abort(); \
       } \
       bInLock = TRUE; \
    }

#define LWREG_UNLOCK_MUTEX(bInLock, mutex) \
    if (bInLock) { \
       int thr_err = pthread_mutex_unlock(mutex); \
       if (thr_err) { \
           LWREG_LOG_ERROR("Failed to unlock mutex. Aborting program"); \
           abort(); \
       } \
       bInLock = FALSE; \
    }

//reader
#define LWREG_LOCK_RWMUTEX_SHARED(bInLock, mutex) \
    if (!bInLock) { \
       int thr_err = pthread_rwlock_rdlock(mutex); \
       if (thr_err) { \
           LWREG_LOG_ERROR("Failed to acquire shared lock on rw mutex. Aborting program"); \
           abort(); \
       } \
       bInLock = TRUE; \
    }

 //writer
#define LWREG_LOCK_RWMUTEX_EXCLUSIVE(bInLock, mutex) \
    if (!bInLock) { \
       int thr_err = pthread_rwlock_wrlock(mutex); \
       if (thr_err) { \
           LWREG_LOG_ERROR("Failed to acquire exclusive lock on rw mutex. Aborting program"); \
           abort(); \
       } \
       bInLock = TRUE; \
    }

#define LWREG_UNLOCK_RWMUTEX(bInLock, mutex) \
    if (bInLock) { \
       int thr_err = pthread_rwlock_unlock(mutex); \
       if (thr_err) { \
           LWREG_LOG_ERROR("Failed to unlock rw mutex. Aborting program"); \
           abort(); \
       } \
       bInLock = FALSE; \
    }

void
RegSrvIpcDestructSession(
    LWMsgSecurityToken* pToken,
    void* pSessionData
    );

LWMsgStatus
RegSrvIpcConstructSession(
    LWMsgSecurityToken* pToken,
    void* pData,
    void** ppSessionData
    );

DWORD
RegSrvApiInit(
    VOID
    );

DWORD
RegSrvApiShutdown(
    VOID
    );

LWMsgDispatchSpec*
RegSrvGetDispatchSpec(
    void
    );

NTSTATUS
RegSrvOpenServer(
    uid_t peerUID,
    gid_t peerGID,
    PHANDLE phServer
    );

void
RegSrvCloseServer(
    HANDLE hServer
    );

NTSTATUS
RegSrvEnumRootKeysW(
    IN HANDLE Handle,
    OUT PWSTR** pppszRootKeys,
    OUT PDWORD pdwNumRootKeys
    );

NTSTATUS
RegSrvCreateKeyEx(
    IN HANDLE Handle,
    IN HKEY hKey,
    IN PCWSTR pSubKey,
    IN DWORD Reserved,
    IN OPTIONAL PWSTR pClass,
    IN DWORD dwOptions,
    IN ACCESS_MASK AccessDesired,
    IN OPTIONAL PSECURITY_DESCRIPTOR_RELATIVE pSecurityDescriptor,
    IN ULONG ulSecDescLen,
    OUT PHKEY phkResult,
    OUT OPTIONAL PDWORD pdwDisposition
    );

NTSTATUS
RegSrvOpenKeyExW(
    IN HANDLE Handle,
    IN HKEY hKey,
    IN OPTIONAL PCWSTR pwszSubKey,
    IN DWORD ulOptions,
    IN ACCESS_MASK AccessDesired,
    OUT PHKEY phkResult
    );

VOID
RegSrvCloseKey(
    HKEY hKey
    );

NTSTATUS
RegSrvDeleteKey(
    HANDLE Handle,
    HKEY hKey,
    PCWSTR pSubKey
    );

NTSTATUS
RegSrvDeleteKeyValue(
    HANDLE Handle,
    HKEY hKey,
    PCWSTR pSubKey,
    PCWSTR pValueName
    );

NTSTATUS
RegSrvDeleteTree(
    HANDLE Handle,
    HKEY hKey,
    PCWSTR lpSubKey
    );

NTSTATUS
RegSrvDeleteValue(
    HANDLE Handle,
    HKEY hKey,
    PCWSTR lpValueName
    );

NTSTATUS
RegSrvEnumKeyExW(
    IN HANDLE Handle,
    IN HKEY hKey,
    IN DWORD dwIndex,
    IN OUT PWSTR pName,
    IN OUT PDWORD pcName,
    IN PDWORD pReserved,
    IN OUT PWSTR pClass,
    IN OUT OPTIONAL PDWORD pcClass,
    OUT OPTIONAL PFILETIME pftLastWriteTime
    );

NTSTATUS
RegSrvEnumValueW(
    IN HANDLE Handle,
    IN HKEY hKey,
    IN DWORD dwIndex,
    OUT PWSTR pValueName, /*buffer hold valueName*/
    IN OUT PDWORD pcchValueName, /*input - buffer pValueName length*/
    IN PDWORD pReserved,
    OUT OPTIONAL PDWORD pType,
    OUT OPTIONAL PBYTE pData,/*buffer hold value content*/
    IN OUT OPTIONAL PDWORD pcbData /*input - buffer pData length*/
    );

NTSTATUS
RegSrvGetValueW(
    IN HANDLE Handle,
    IN HKEY hKey,
    IN OPTIONAL PCWSTR pSubKey,
    IN OPTIONAL PCWSTR pValue,
    IN OPTIONAL REG_DATA_TYPE_FLAGS Flags,
    OUT PDWORD pdwType,
    OUT PBYTE pData,
    IN OUT PDWORD pcbData
    );

NTSTATUS
RegSrvQueryInfoKeyW(
    HANDLE Handle,
    HKEY hKey,
    PWSTR pClass,
    PDWORD pcClass,
    PDWORD pReserved,
    PDWORD pcSubKeys,
    PDWORD pcMaxSubKeyLen,
    PDWORD pcMaxClassLen,
    PDWORD pcValues,
    PDWORD pcMaxValueNameLen,
    PDWORD pcMaxValueLen,
    PDWORD pcbSecurityDescriptor,
    PFILETIME pftLastWriteTime
    );

NTSTATUS
RegSrvQueryMultipleValues(
    IN HANDLE Handle,
    IN HKEY hKey,
    IN OUT PVALENT pVal_list,
    IN DWORD num_vals,
    OUT OPTIONAL PWSTR pValue,
    OUT OPTIONAL PDWORD pdwTotalsize
    );

NTSTATUS
RegSrvSetValueExW(
    IN HANDLE Handle,
    IN HKEY hKey,
    IN OPTIONAL PCWSTR pValueName,
    IN DWORD Reserved,
    IN DWORD dwType,
    IN const BYTE *pData,
    DWORD cbData
    );

NTSTATUS
RegSrvSetKeySecurity(
    IN HANDLE Handle,
    IN HKEY hKey,
    IN SECURITY_INFORMATION SecurityInformation,
    IN PSECURITY_DESCRIPTOR_RELATIVE pSecurityDescriptor,
    IN ULONG ulSecDescLength
    );

NTSTATUS
RegSrvGetKeySecurity(
    IN HANDLE Handle,
    IN HKEY hKey,
    IN SECURITY_INFORMATION SecurityInformation,
    OUT PSECURITY_DESCRIPTOR_RELATIVE pSecurityDescriptor,
    IN OUT PULONG pulSecDescLength
    );




// Key context (key handle) utility functions
BOOLEAN
RegSrvIsValidKeyName(
    PCWSTR pwszKeyName
    );

void
RegSrvSafeFreeKeyContext(
    IN PREG_KEY_CONTEXT pKeyResult
    );

DWORD
RegSrvGetKeyRefCount(
    IN PREG_KEY_CONTEXT pKeyResult
    );

void
RegSrvResetSubKeyInfo(
    IN OUT PREG_KEY_CONTEXT pKeyResult
    );

BOOLEAN
RegSrvHasSubKeyInfo(
    IN PREG_KEY_CONTEXT pKeyResult
    );

DWORD
RegSrvSubKeyNum(
    IN PREG_KEY_CONTEXT pKeyResult
    );

size_t
RegSrvSubKeyNameMaxLen(
    IN PREG_KEY_CONTEXT pKeyResult
    );

PCWSTR
RegSrvSubKeyName(
    IN PREG_KEY_CONTEXT pKeyResult,
    IN DWORD dwIndex
    );

BOOLEAN
RegSrvHasSecurityDescriptor(
    IN PREG_KEY_CONTEXT pKeyResult
    );

ULONG
RegSrvGetKeySecurityDescriptorSize(
    IN PREG_KEY_CONTEXT pKeyResult
    );

NTSTATUS
RegSrvGetKeySecurityDescriptor_inlock(
    IN PREG_KEY_CONTEXT pKeyResult,
    IN OUT PSECURITY_DESCRIPTOR_RELATIVE pSecurityDescriptor,
    IN ULONG ulSecDescRelLen
    );

NTSTATUS
RegSrvGetKeySecurityDescriptor(
    IN PREG_KEY_CONTEXT pKeyResult,
    IN OUT PSECURITY_DESCRIPTOR_RELATIVE pSecurityDescriptor,
    IN ULONG ulSecDescRelLen
    );

NTSTATUS
RegSrvSetKeySecurityDescriptor_inlock(
    IN PREG_KEY_CONTEXT pKeyResult,
    IN PSECURITY_DESCRIPTOR_RELATIVE pSecurityDescriptor,
    IN ULONG ulSecDescRelLen
    );

NTSTATUS
RegSrvSetKeySecurityDescriptor(
    IN PREG_KEY_CONTEXT pKeyResult,
    IN PSECURITY_DESCRIPTOR_RELATIVE pSecurityDescriptor,
    IN ULONG ulSecDescRelLen
    );

void
RegSrvResetValueInfo(
    IN OUT PREG_KEY_CONTEXT pKey
    );

BOOLEAN
RegSrvHasValueInfo(
    IN PREG_KEY_CONTEXT pKeyResult
    );

DWORD
RegSrvValueNum(
    IN PREG_KEY_CONTEXT pKeyResult
    );

size_t
RegSrvMaxValueNameLen(
    IN PREG_KEY_CONTEXT pKeyResult
    );

size_t
RegSrvMaxValueLen(
    IN PREG_KEY_CONTEXT pKeyResult
    );

PCWSTR
RegSrvValueName(
    IN PREG_KEY_CONTEXT pKeyResult,
    DWORD dwIndex
    );

void
RegSrvValueContent(
    IN PREG_KEY_CONTEXT pKeyResult,
    DWORD dwIndex,
    PBYTE* ppValue,
    PDWORD pdwValueLen
    );

REG_DATA_TYPE
RegSrvValueType(
    IN PREG_KEY_CONTEXT pKeyResult,
    DWORD dwIndex
    );

//Registry ACL check

NTSTATUS
RegSrvCreateAccessToken(
    uid_t uid,
    gid_t gid,
    PACCESS_TOKEN* ppToken
    );

NTSTATUS
RegSrvAccessCheckKey(
	IN PACCESS_TOKEN pToken,
    IN PSECURITY_DESCRIPTOR_RELATIVE pSecDescRel,
    IN ULONG ulSecDescRelLen,
    IN ACCESS_MASK AccessDesired,
    OUT ACCESS_MASK *psamGranted
    );

NTSTATUS
RegSrvAccessCheckKeyHandle(
    IN PREG_KEY_HANDLE pKeyHandle,
    IN ACCESS_MASK AccessRequired
    );

NTSTATUS
RegSrvCreateDefaultSecDescRel(
    IN OUT PSECURITY_DESCRIPTOR_RELATIVE* ppSecDescRel,
    IN OUT PULONG pulSecDescLength
    );

// Registry Security related utility functions
VOID
RegSrvFreeAbsoluteSecurityDescriptor(
    IN OUT PSECURITY_DESCRIPTOR_ABSOLUTE *ppSecDesc
    );

#endif // __REGSERVER_H_
