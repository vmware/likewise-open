/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

/*
 * Copyright Likewise Software    2004-2008
 * All rights reserved.
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the license, or (at
 * your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser
 * General Public License for more details.  You should have received a copy
 * of the GNU Lesser General Public License along with this program.  If
 * not, see <http://www.gnu.org/licenses/>.
 *
 * LIKEWISE SOFTWARE MAKES THIS SOFTWARE AVAILABLE UNDER OTHER LICENSING
 * TERMS AS WELL.  IF YOU HAVE ENTERED INTO A SEPARATE LICENSE AGREEMENT
 * WITH LIKEWISE SOFTWARE, THEN YOU MAY ELECT TO USE THE SOFTWARE UNDER THE
 * TERMS OF THAT SOFTWARE LICENSE AGREEMENT INSTEAD OF THE TERMS OF THE GNU
 * LESSER GENERAL PUBLIC LICENSE, NOTWITHSTANDING THE ABOVE NOTICE.  IF YOU
 * HAVE QUESTIONS, OR WISH TO REQUEST A COPY OF THE ALTERNATE LICENSING
 * TERMS OFFERED BY LIKEWISE SOFTWARE, PLEASE CONTACT LIKEWISE SOFTWARE AT
 * license@likewisesoftware.com
 */



/*
 * Copyright (C) Likewise Software. All rights reserved.
 *
 * Module Name:
 *
 *        lsautils.h
 *
 * Abstract:
 *
 *        Likewise Security and Authentication Subsystem (LSASS) system utilities
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 */

#ifndef __LSA_UTILS_H__
#define __LSA_UTILS_H__

#include "lsaipc.h"
#include "lsalist.h"
#include <lw/ntstatus.h>
#include elw/security-types.h>

#if !defined(HAVE_STRTOLL)

long long int
strtoll(
    const char* nptr,
    char**      endptr,
    int         base
    );

#endif /* defined(HAVE_STRTOLL) */

#if !defined(HAVE_STRTOULL)

unsigned long long int
strtoull(
    const char* nptr,
    char**      endptr,
    int         base
    );

#endif /* defined(HAVE_STRTOULL) */

DWORD
LwInitializeStringBuffer(
        LSA_STRING_BUFFER *pBuffer,
        size_t sCapacity);

DWORD
LwAppendStringBuffer(
        LSA_STRING_BUFFER *pBuffer,
        PCSTR pszAppend);

void
LwFreeStringBufferContents(
        LSA_STRING_BUFFER *pBuffer);

DWORD
LwAllocateMemory(
    DWORD dwSize,
    PVOID * ppMemory
    );

DWORD
LwReallocMemory(
    PVOID  pMemory,
    PVOID * ppNewMemory,
    DWORD dwSize
    );

DWORD
LwAppendAndFreePtrs(
    IN OUT PDWORD pdwDestCount,
    IN OUT PVOID** pppDestPtrArray,
    IN OUT PDWORD pdwAppendCount,
    IN OUT PVOID** pppAppendPtrArray
    );

void
LwFreeMemory(
    PVOID pMemory
    );

DWORD
LwAllocateString(
    PCSTR pszInputString,
    PSTR *ppszOutputString
    );

void
LwFreeString(
    PSTR pszString
    );

void
LwStripWhitespace(
    PSTR pszString,
    BOOLEAN bLeading,
    BOOLEAN bTrailing
    );

DWORD
LwStrIsAllSpace(
    PCSTR pszString,
    PBOOLEAN pbIsAllSpace
    );

void
LwStrToUpper(
    PSTR pszString
    );

void
LwStrnToUpper(
    PSTR  pszString,
    DWORD dwLen
    );

void
LwStrToLower(
    PSTR pszString
    );

void
LwStrnToLower(
    PSTR  pszString,
    DWORD dwLen
    );

DWORD
LwEscapeString(
    PSTR pszOrig,
    PSTR * ppszEscapedString
    );

DWORD
LwStrndup(
    PCSTR pszInputString,
    size_t size,
    PSTR * ppszOutputString
    );

DWORD
LwAllocateStringPrintf(
    PSTR* ppszOutputString,
    PCSTR pszFormat,
    ...
    );

DWORD
LwAllocateStringPrintfV(
    PSTR*   ppszOutputString,
    PCSTR   pszFormat,
    va_list args
    );

VOID
LwStrCharReplace(
    PSTR pszStr,
    CHAR oldCh,
    CHAR newCh);

// If pszInputString == NULL, then *ppszOutputString = NULL
DWORD
LwStrDupOrNull(
    PCSTR pszInputString,
    PSTR *ppszOutputString
    );


// If pszInputString == NULL, return "", else return pszInputString
// The return value does not need to be freed.
PCSTR
LwEmptyStrForNull(
    PCSTR pszInputString
    );


void
LwStrChr(
    PCSTR pszInputString,
    CHAR c,
    PSTR *pszOutputString
    );

void
LwFreeStringArray(
    PSTR * ppStringArray,
    DWORD dwCount
    );

void
LwFreeNullTerminatedStringArray(
    PSTR * ppStringArray
    );

VOID
LwPrincipalNonRealmToLower(
    IN OUT PSTR pszPrincipal
    );

VOID
LwPrincipalRealmToUpper(
    IN OUT PSTR pszPrincipal
    );

DWORD
LwBitVectorCreate(
    DWORD dwNumBits,
    PLSA_BIT_VECTOR* ppBitVector
    );

VOID
LwBitVectorFree(
    PLSA_BIT_VECTOR pBitVector
    );

BOOLEAN
LwBitVectorIsSet(
    PLSA_BIT_VECTOR pBitVector,
    DWORD           iBit
    );

DWORD
LwBitVectorSetBit(
    PLSA_BIT_VECTOR pBitVector,
    DWORD           iBit
    );

DWORD
LwBitVectorUnsetBit(
    PLSA_BIT_VECTOR pBitVector,
    DWORD           iBit
    );

VOID
LwBitVectorReset(
    PLSA_BIT_VECTOR pBitVector
    );

DWORD
LwDLinkedListPrepend(
    PDLINKEDLIST* ppList,
    PVOID        pItem
    );

DWORD
LwDLinkedListAppend(
    PDLINKEDLIST* ppList,
    PVOID        pItem
    );

BOOLEAN
LwDLinkedListDelete(
    PDLINKEDLIST* ppList,
    PVOID        pItem
    );

VOID
LwDLinkedListForEach(
    PDLINKEDLIST          pList,
    PFN_DLINKEDLIST_FUNC pFunc,
    PVOID                pUserData
    );

VOID
LwDLinkedListFree(
    PDLINKEDLIST pList
    );

DWORD
LwStackPush(
    PVOID pItem,
    PLSA_STACK* ppStack
    );

PVOID
LwStackPop(
    PLSA_STACK* ppStack
    );

PVOID
LwStackPeek(
    PLSA_STACK pStack
    );

DWORD
LwStackForeach(
    PLSA_STACK pStack,
    PFN_LSA_FOREACH_STACK_ITEM pfnAction,
    PVOID pUserData
    );

PLSA_STACK
LwStackReverse(
    PLSA_STACK pStack
    );

VOID
LwStackFree(
    PLSA_STACK pStack
    );

DWORD
LwHashCreate(
    size_t sTableSize,
    LSA_HASH_KEY_COMPARE fnComparator,
    LSA_HASH_KEY fnHash,
    LSA_HASH_FREE_ENTRY fnFree, //optional
    LSA_HASH_COPY_ENTRY fnCopy, //optional
    LSA_HASH_TABLE** ppResult
    );

void
LwHashSafeFree(
    LSA_HASH_TABLE** ppResult
    );

DWORD
LwHashSetValue(
    LSA_HASH_TABLE *pTable,
    PVOID  pKey,
    PVOID  pValue
    );

//Returns ENOENT if pKey is not in the table
DWORD
LwHashGetValue(
    LSA_HASH_TABLE *pTable,
    PCVOID  pKey,
    PVOID* ppValue
    );

BOOLEAN
LwHashExists(
    IN PLSA_HASH_TABLE pTable,
    IN PCVOID pKey
    );

DWORD
LwHashCopy(
    IN  LSA_HASH_TABLE *pTable,
    OUT LSA_HASH_TABLE **ppResult
    );

//Invalidates all iterators
DWORD
LwHashResize(
    LSA_HASH_TABLE *pTable,
    size_t sTableSize
    );

DWORD
LwHashGetIterator(
    LSA_HASH_TABLE *pTable,
    LSA_HASH_ITERATOR *pIterator
    );

// returns NULL after passing the last entry
LSA_HASH_ENTRY *
LwHashNext(
    LSA_HASH_ITERATOR *pIterator
    );

DWORD
LwHashRemoveKey(
    LSA_HASH_TABLE *pTable,
    PVOID  pKey
    );

int
LwHashCaselessStringCompare(
    PCVOID str1,
    PCVOID str2
    );

size_t
LwHashCaselessString(
    PCVOID str
    );

VOID
LwHashFreeStringKey(
    IN OUT const LSA_HASH_ENTRY *pEntry
    );

DWORD
LwRemoveFile(
    PCSTR pszPath
    );

DWORD
LwCheckFileExists(
    PCSTR pszPath,
    PBOOLEAN pbFileExists
    );

DWORD
LwCheckSockExists(
    PSTR pszPath,
    PBOOLEAN pbFileExists
    );

DWORD
LwCheckLinkExists(
    PSTR pszPath,
    PBOOLEAN pbFileExists
    );

DWORD
LwCheckFileOrLinkExists(
    PSTR pszPath,
    PBOOLEAN pbExists
    );

DWORD
LwMoveFile(
    PCSTR pszSrcPath,
    PCSTR pszDstPath
    );

DWORD
LwChangePermissions(
    PCSTR pszPath,
    mode_t dwFileMode
    );

DWORD
LwChangeOwner(
    PCSTR pszPath,
    uid_t uid,
    gid_t gid
    );

DWORD
LwChangeOwnerAndPermissions(
    PCSTR pszPath,
    uid_t uid,
    gid_t gid,
    mode_t dwFileMode
    );

DWORD
LwGetCurrentDirectoryPath(
    PSTR* ppszPath
    );

DWORD
LwChangeDirectory(
    PSTR pszPath
    );

DWORD
LwRemoveDirectory(
    PSTR pszPath
    );

DWORD
LwCopyDirectory(
    PCSTR pszSourceDirPath,
    uid_t ownerUid,
    gid_t ownerGid,
    PCSTR pszDestDirPath
    );

DWORD
LwCheckDirectoryExists(
    PCSTR pszPath,
    PBOOLEAN pbDirExists
    );

DWORD
LwCreateDirectory(
    PCSTR pszPath,
    mode_t dwFileMode
    );

DWORD
LwGetDirectoryFromPath(
    IN PCSTR pszPath,
    OUT PSTR* ppszDir
    );

DWORD
LwCopyFileWithPerms(
    PCSTR pszSrcPath,
    PCSTR pszDstPath,
    mode_t dwPerms
    );

DWORD
LwGetOwnerAndPermissions(
    PCSTR pszSrcPath,
    uid_t * uid,
    gid_t * gid,
    mode_t * mode
    );

DWORD
LwCopyFileWithOriginalPerms(
    PCSTR pszSrcPath,
    PCSTR pszDstPath
    );

DWORD
LwBackupFile(
    PCSTR pszPath
    );

DWORD
LwGetSymlinkTarget(
   PCSTR pszPath,
   PSTR* ppszTargetPath
   );

DWORD
LwCreateSymlink(
   PCSTR pszOldPath,
   PCSTR pszNewPath
   );

DWORD
LwGetMatchingFilePathsInFolder(
    PCSTR pszDirPath,
    PCSTR pszFileNameRegExp,
    PSTR** pppszHostFilePaths,
    PDWORD pdwNPaths
    );

DWORD
LwGetPrefixDirPath(
    PSTR* ppszPath
    );

DWORD
LwGetLibDirPath(
    PSTR* ppszPath
    );

DWORD
LwValidateGroupInfoLevel(
    DWORD dwGroupInfoLevel
    );

DWORD
LwValidateGroupName(
    PCSTR pszGroupName
    );

DWORD
LwValidateGroupInfo(
    PVOID pGroupInfo,
    DWORD dwGroupInfoLevel
    );

DWORD
LwValidateUserInfo(
    PVOID pUserInfo,
    DWORD dwUserInfoLevel
    );

DWORD
LwValidateUserInfoLevel(
    DWORD dwUserInfoLevel
    );

DWORD
LwValidateUserName(
    PCSTR pszName
    );

#define LSA_DOMAIN_SEPARATOR_DEFAULT    '\\'

CHAR
LwGetDomainSeparator(
    VOID
    );

DWORD
LwSetDomainSeparator(
    CHAR chValue
    );

DWORD
LwCrackDomainQualifiedName(
    PCSTR pszId,
    PCSTR pszDefaultDomain,
    PLSA_LOGIN_NAME_INFO* ppNameInfo
    );

void
LwFreeNameInfo(
    PLSA_LOGIN_NAME_INFO pNameInfo
    );

DWORD
LwAllocateGroupInfo(
    PVOID *ppDstGroupInfo,
    DWORD dwLevel,
    PVOID pSrcGroupInfo
    );

DWORD
LwBuildUserModInfo(
    uid_t uid,
    PLSA_USER_MOD_INFO* ppUserModInfo
    );

DWORD
LwModifyUser_EnableUser(
    PLSA_USER_MOD_INFO pUserModInfo,
    BOOLEAN bValue
    );

DWORD
LwModifyUser_DisableUser(
    PLSA_USER_MOD_INFO pUserModInfo,
    BOOLEAN bValue
    );

DWORD
LwModifyUser_Unlock(
    PLSA_USER_MOD_INFO pUserModInfo,
    BOOLEAN bValue
    );

DWORD
LwModifyUser_AddToGroups(
    PLSA_USER_MOD_INFO pUserModInfo,
    PCSTR pszGroupList
    );

DWORD
LwModifyUser_RemoveFromGroups(
    PLSA_USER_MOD_INFO pUserModInfo,
    PCSTR pszGroupList
    );

DWORD
LwModifyUser_ChangePasswordAtNextLogon(
    PLSA_USER_MOD_INFO pUserModInfo,
    BOOLEAN bValue
    );

DWORD
LwModifyUser_SetPasswordNeverExpires(
    PLSA_USER_MOD_INFO pUserModInfo,
    BOOLEAN bValue
    );

DWORD
LwModifyUser_SetPasswordMustExpire(
    PLSA_USER_MOD_INFO pUserModInfo,
    BOOLEAN bValue
    );

DWORD
LwModifyUser_SetAccountExpiryDate(
    PLSA_USER_MOD_INFO pUserModInfo,
    PCSTR pszDate
    );

void
LwFreeUserModInfo(
    PLSA_USER_MOD_INFO pUserModInfo
    );

DWORD
LwBuildGroupModInfo(
    gid_t gid,
    PLSA_GROUP_MOD_INFO *ppGroupModInfo
    );

DWORD
LwModifyGroup_AddMembers(
    PLSA_GROUP_MOD_INFO pGroupModInfo,
    PVOID pData
    );

DWORD
LwModifyGroup_RemoveMembers(
    PLSA_GROUP_MOD_INFO pGroupModInfo,
    PVOID pData
    );

void
LwFreeGroupModInfo(
    PLSA_GROUP_MOD_INFO pGroupModInfo
    );

void
LwFreeIpcNameSidsList(
    PLSA_FIND_NAMES_BY_SIDS pNameSidsList
    );

void
LwFreeIpcUserInfoList(
    PLSA_USER_INFO_LIST pUserIpcInfoList
    );

VOID
LwSrvFreeIpcMetriPack(
    PLSA_METRIC_PACK pMetricPack
    );

void
LwFreeIpcGroupInfoList(
    PLSA_GROUP_INFO_LIST pGroupIpcInfoList
    );

void
LwFreeIpcNssArtefactInfoList(
    PLSA_NSS_ARTEFACT_INFO_LIST pNssArtefactIpcInfoList
    );

DWORD
LwParseConfigFile(
    PCSTR                     pszFilePath,
    DWORD                     dwOptions,
    PFNCONFIG_START_SECTION   pfnStartSectionHandler,
    PFNCONFIG_COMMENT         pfnCommentHandler,
    PFNCONFIG_NAME_VALUE_PAIR pfnNameValuePairHandler,
    PFNCONFIG_END_SECTION     pfnEndSectionHandler,
    PVOID                     pData
    );

DWORD
LwDnsGetHostInfo(
    PSTR* ppszHostname
    );

VOID
LwDnsFreeFQDNList(
    PDNS_FQDN* ppFQDNList,
    DWORD dwNFQDN
    );

VOID
LwDnsFreeFQDN(
    PDNS_FQDN pFQDN
    );

DWORD
LwInitLogging(
    PCSTR         pszProgramName,
    LwLogTarget  logTarget,
    LwLogLevel   maxAllowedLogLevel,
    PCSTR         pszPath
    );

DWORD
LwLogGetInfo(
    PLSA_LOG_INFO* ppLogInfo
    );

DWORD
LwLogSetInfo(
    PLSA_LOG_INFO pLogInfo
    );

VOID
LwLogMessage(
    PFN_LSA_LOG_MESSAGE pfnLogger,
    HANDLE hLog,
    LwLogLevel logLevel,
    PCSTR  pszFormat,
    ...
    );

void
lsass_vsyslog(
    int priority,
    const char *format,
    va_list ap
    );

DWORD
LwShutdownLogging(
    VOID
    );

DWORD
LwValidateLogLevel(
    DWORD dwLogLevel
    );

DWORD
LwTraceInitialize(
    VOID
    );

BOOLEAN
LwTraceIsFlagSet(
    DWORD dwTraceFlag
    );

BOOLEAN
LwTraceIsAllowed(
    DWORD dwTraceFlags[],
    DWORD dwNumFlags
    );

DWORD
LwTraceSetFlag(
    DWORD dwTraceFlag
    );

DWORD
LwTraceUnsetFlag(
    DWORD dwTraceFlag
    );

VOID
LwTraceShutdown(
    VOID
    );

VOID
LwTraceShutdown(
    VOID
    );

DWORD
LwAllocSecurityIdentifierFromBinary(
    UCHAR* sidBytes,
    DWORD dwSidBytesLength,
    PLSA_SECURITY_IDENTIFIER* ppSecurityIdentifier
    );

DWORD
LwAllocSecurityIdentifierFromString(
    PCSTR pszSidString,
    PLSA_SECURITY_IDENTIFIER* ppSecurityIdentifier
    );

VOID
LwFreeSecurityIdentifier(
    PLSA_SECURITY_IDENTIFIER pSecurityIdentifier
    );

DWORD
LwGetSecurityIdentifierRid(
    PLSA_SECURITY_IDENTIFIER pSecurityIdentifier,
    PDWORD pdwRid
    );

DWORD
LwSetSecurityIdentifierRid(
    PLSA_SECURITY_IDENTIFIER pSecurityIdentifier,
    DWORD dwRid
    );

DWORD
LwReplaceSidRid(
    IN PCSTR pszSid,
    IN DWORD dwNewRid,
    OUT PSTR* ppszNewSid
    );

DWORD
LwGetSecurityIdentifierHashedRid(
    PLSA_SECURITY_IDENTIFIER pSecurityIdentifier,
    PDWORD dwHashedRid
    );

// The UID is a DWORD constructued using a non-cryptographic hash
// of the User's domain SID and user RID.
DWORD
LwHashSecurityIdentifierToId(
    IN PLSA_SECURITY_IDENTIFIER pSecurityIdentifier,
    OUT PDWORD pdwId
    );

DWORD
LwHashSidStringToId(
    IN PCSTR pszSidString,
    OUT PDWORD pdwId
    );

DWORD
LwGetSecurityIdentifierBinary(
    PLSA_SECURITY_IDENTIFIER pSecurityIdentifier,
    UCHAR** ppucSidBytes,
    DWORD* pdwSidBytesLength
    );

DWORD
LwGetSecurityIdentifierString(
    PLSA_SECURITY_IDENTIFIER pSecurityIdentifier,
    PSTR* ppszSidStr
    );

DWORD
LwSidBytesToString(
    UCHAR* pucSidBytes,
    DWORD dwSidBytesLength,
    PSTR* pszSidString
    );

DWORD
LwGetDomainSecurityIdentifier(
    PLSA_SECURITY_IDENTIFIER pSecurityIdentifier,
    PLSA_SECURITY_IDENTIFIER* ppDomainSID
    );

DWORD
LwHexStrToByteArray(
    IN PCSTR pszHexString,
    IN OPTIONAL DWORD* pdwHexStringLength,
    OUT UCHAR** ppucByteArray,
    OUT DWORD* pdwByteArrayLength
    );

DWORD
LwByteArrayToHexStr(
    IN UCHAR* pucByteArray,
    IN DWORD dwByteArrayLength,
    OUT PSTR* ppszHexString
    );

DWORD
LwByteArrayToLdapFormatHexStr(
    IN UCHAR* pucByteArray,
    IN DWORD dwByteArrayLength,
    OUT PSTR* ppszHexString
    );

DWORD
LwSidStrToLdapFormatHexStr(
    IN PCSTR pszSid,
    OUT PSTR* ppszHexSid
    );

int
LwStrError(
    int errnum,
    char *pszBuf,
    size_t buflen
    );

VOID
LwFreeDCInfo(
    PLSA_DC_INFO pDCInfo
    );

VOID
LwFreeDomainInfoArray(
    DWORD dwNumDomains,
    PLSA_TRUSTED_DOMAIN_INFO pDomainInfoArray
    );

VOID
LwFreeDomainInfo(
    PLSA_TRUSTED_DOMAIN_INFO pDomainInfo
    );

VOID
LwFreeDomainInfoContents(
    PLSA_TRUSTED_DOMAIN_INFO pDomainInfo
    );

DWORD
LwNISGetNicknames(
    PCSTR         pszNicknameFilePath,
    PDLINKEDLIST* ppNicknameList
    );

PCSTR
LwNISLookupAlias(
    PDLINKEDLIST pNicknameList,
    PCSTR pszAlias
    );

VOID
LwNISFreeNicknameList(
    PDLINKEDLIST pNicknameList
    );

DWORD
LwCacheNew(
    DWORD dwNumKeys,
    DWORD dwNumBuckets,
    PFLSA_CACHE_HASH pfHash,
    PFLSA_CACHE_EQUAL pfEqual,
    PFLSA_CACHE_GETKEY pfGetKey,
    PFLSA_CACHE_MISS pfMiss,
    PFLSA_CACHE_KICK pfKick,
    PVOID pData,
    PLSA_CACHE* ppCache
    );

DWORD
LwCacheInsert(
    PLSA_CACHE pCache,
    PVOID pEntry
    );

DWORD
LwCacheRemove(
    PLSA_CACHE pCache,
    PVOID pEntry
    );

DWORD
LwCacheLookup(
    PLSA_CACHE pCache,
    PVOID pkey,
    DWORD dwIndex,
    PVOID* ppEntry
    );

DWORD
LwCacheFlush(
    PLSA_CACHE pCache
    );

VOID
LwCacheDelete(
    PLSA_CACHE pCache
    );

DWORD
LwTranslateLwMsgError(
        LWMsgAssoc *pAssoc,
        DWORD dwMsgError,
        const char *file,
        int line
        );

DWORD
LwNtStatusToLwError(
    IN NTSTATUS Status
    );

NTSTATUS
LwLwErrorToNtStatus(
    IN DWORD LwError
    );

DWORD
LwAllocateCStringFromSid(
    OUT PSTR* ppszStringSid,
    IN PSID pSid
    );

DWORD
LwAllocateSidFromCString(
    OUT PSID* ppSid,
    IN PCSTR pszStringSid
    );

DWORD
LwAllocateSidAppendRid(
    OUT PSID* ppSid,
    IN PSID pDomainSid,
    IN ULONG Rid
    );

#endif /* __LSA_UTILS_H__ */


